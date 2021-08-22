#include "gfx/engine.hpp"

#include "config.hpp"

#include <cassert>
#include "Tracy.hpp"
#include "TracyVulkan.hpp"
#include "volk.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "base/error.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/modules/cubeFilter.hpp"
#include "gfx/modules/indirect.hpp"
#include "gfx/modules/tonemap.hpp"
#include "gfx/modules/forward.hpp"
#include "gfx/modules/bloom.hpp"
#include "gfx/base.hpp"
#include "main.hpp"

namespace minote::gfx {

using namespace base;
using namespace std::string_literals;

Engine::Engine(sys::Vulkan& _vk, MeshList&& _meshList):
	m_vk(_vk) {
	
	ZoneScoped;
	
	auto ifc = m_vk.context->begin();
	auto ptc = ifc.begin();
	
	// Compile pipelines
	
	CubeFilter::compile(ptc);
	Atmosphere::compile(ptc);
	Tonemap::compile(ptc);
	Bloom::compile(ptc);
	
	// Initialize internal resources
	m_swapchainDirty = false;
	
	m_imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	// Begin imgui frame so that first-frame calls succeed
	ImGui::NewFrame();
	
	m_meshes = std::move(_meshList).upload(ptc, "Meshes");
	m_atmosphere.upload(ptc, "Earth", Atmosphere::Params::earth());
	m_objects = ObjectPool();
	
	m_iblUnfiltered.emplace(ptc, "ibl_unfiltered", 256, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferSrc);
	m_iblFiltered.emplace(ptc, "ibl_filtered", 256, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferDst);
	
	// Perform precalculations
	auto precalc = m_atmosphere.precalculate();
	
	// Finalize
	
	ptc.wait_all_transfers();
	
	auto erg = std::move(precalc).link(ptc);
	vuk::execute_submit_and_wait(ptc, std::move(erg));
	
	L_INFO("Graphics engine initialized");
	
}

Engine::~Engine() {
	
	ZoneScoped;
	
	m_vk.context->wait_idle();
	
	auto ifc = m_vk.context->begin();
	auto ptc = ifc.begin();
	
	m_iblFiltered.reset();
	m_iblUnfiltered.reset();
	
	m_atmosphere.cleanup(ptc);
	m_objects.reset();
	m_meshes.cleanup(ptc);
	m_imguiData.fontTex.view.reset();
	m_imguiData.fontTex.image.reset();
	
	L_INFO("Graphics engine cleaned up");
	
}

void Engine::render(bool _repaint) {
	
	ZoneScoped;
	
	// Quit if repaint needed
	if (m_swapchainDirty) return;
	
	// Lock the renderer
	m_renderLock.lock();
	defer { m_renderLock.unlock(); };
	
	// Prepare per-frame data
	
	// Basic scene properties
	auto viewport = uvec2{m_vk.swapchain->extent.width, m_vk.swapchain->extent.height};
	m_world.projection = perspective(VerticalFov, f32(viewport.x()) / f32(viewport.y()), NearPlane);
	m_world.view = m_camera.transform();
	m_world.viewProjection = m_world.projection * m_world.view;
	m_world.viewProjectionInverse = inverse(m_world.viewProjection);
	m_world.viewportSize = viewport;
	m_world.cameraPos = m_camera.position;
	
	// Sun properties
	static auto sunPitch = 7.2_deg;
	static auto sunYaw = 30.0_deg;
	ImGui::SliderAngle("Sun pitch", &sunPitch, -8.0f, 60.0f, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderAngle("Sun yaw", &sunYaw, -180.0f, 180.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	m_world.sunDirection =
		mat3::rotate({0.0f, 0.0f, 1.0f}, sunYaw) *
		mat3::rotate({0.0f, -1.0f, 0.0f}, sunPitch) *
		vec3{1.0f, 0.0f, 0.0f};
	
	static auto sunIlluminance = 4.0f;
	ImGui::SliderFloat("Sun illuminance", &sunIlluminance, 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
	m_world.sunIlluminance = vec3(sunIlluminance);
	
	// Begin frame
	
	auto ifc = m_vk.context->begin();
	auto ptc = ifc.begin();
	
	// Initialize modules
	
	auto worldBuf = m_world.upload(ptc, "world");
	auto indirect = Indirect(ptc, *m_objects, m_meshes);
	auto sky = Sky(ptc, m_atmosphere);
	auto forward = Forward(ptc, viewport);
	
	// Set up the rendergraph
	
	auto rg = vuk::RenderGraph();
	
	rg.append(sky.calculate(worldBuf, m_camera));
	rg.append(sky.drawCubemap(worldBuf, *m_iblUnfiltered));
	rg.append(CubeFilter::apply("IBL", *m_iblUnfiltered, *m_iblFiltered));
	rg.append(indirect.sortAndCull(m_world, m_meshes));
	rg.append(forward.zPrepass(worldBuf, indirect, m_meshes));
	rg.append(forward.draw(worldBuf, indirect, m_meshes, sky, *m_iblFiltered));
	rg.append(sky.draw(worldBuf, forward.Color_n, forward.Depth_n, viewport));
	rg.append(Bloom::apply(ptc, "Fb bloom", forward.Color_n, forward.size()));
	rg.append(Tonemap::apply(forward.Color_n, "swapchain", viewport));
	
	ImGui::Render();
	ImGui_ImplVuk_Render(ptc, rg, "swapchain", "swapchain", m_imguiData, ImGui::GetDrawData());
	
	rg.attach_swapchain("swapchain", m_vk.swapchain, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	
	rg.add_tracy_collection();
	
	// Attach used resources
	
	m_iblUnfiltered->attach(rg, vuk::eNone, vuk::eNone);
	m_iblFiltered->attach(rg, vuk::eNone, vuk::eNone);
	
	// Acquire swapchain image
	
	auto presentSem = ptc.acquire_semaphore();
	auto swapchainImageIndex = u32(0);
	auto error = vkAcquireNextImageKHR(m_vk.device.device, m_vk.swapchain->swapchain,
		UINT64_MAX, presentSem, VK_NULL_HANDLE, &swapchainImageIndex);
	if (error == VK_ERROR_OUT_OF_DATE_KHR) {
		
		m_swapchainDirty = true;
		return; // Requires repaint
		
	}
	if (error != VK_SUCCESS && error != VK_SUBOPTIMAL_KHR) // Unknown result
		throw runtime_error_fmt("Unable to acquire swapchain image: error {}", error);
	
	// Build and submit the rendergraph
	
	auto erg = std::move(rg).link(ptc);
	auto commandBuffer = erg.execute(ptc, {{m_vk.swapchain, swapchainImageIndex}});
	
	auto renderSem = ptc.acquire_semaphore();
	auto waitStage = VkPipelineStageFlags(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	auto submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &presentSem,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderSem};
	m_vk.context->submit_graphics(submitInfo, ptc.acquire_fence());
	
	// Present to screen
	
	auto presentInfo = VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderSem,
		.swapchainCount = 1,
		.pSwapchains = &m_vk.swapchain->swapchain,
		.pImageIndices = &swapchainImageIndex};
	auto result = vkQueuePresentKHR(m_vk.context->graphics_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		m_swapchainDirty = true; // No need to return, only cleanup is left
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw runtime_error_fmt("Unable to present to the screen: error {}", result);
	
	// Clean up
	
	worldBuf.recycle(ptc);
	
	ImGui::NewFrame();
	FrameMark;
	
}

void Engine::refreshSwapchain(uvec2 _newSize) {
	
	std::lock_guard lock(m_renderLock);
	
	for (auto iv: m_vk.swapchain->image_views)
		m_vk.context->enqueue_destroy(iv);
	
	auto newSwapchain = m_vk.context->add_swapchain(m_vk.createSwapchain(_newSize, m_vk.swapchain->swapchain));
	m_vk.context->enqueue_destroy(m_vk.swapchain->swapchain);
	m_vk.context->remove_swapchain(m_vk.swapchain);
	m_vk.swapchain = newSwapchain;
	m_swapchainDirty = false;
	
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	
}

}
