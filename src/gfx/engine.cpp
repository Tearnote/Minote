#include "gfx/engine.hpp"

#include "config.hpp"

#include <cassert>
#include "volk.h"
#include "Tracy.hpp"
#include "TracyVulkan.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "base/error.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/effects/cubeFilter.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/effects/indirect.hpp"
#include "gfx/effects/tonemap.hpp"
#include "gfx/effects/forward.hpp"
#include "gfx/effects/bloom.hpp"
#include "gfx/util.hpp"
#include "main.hpp"

namespace minote::gfx {

using namespace base;
using namespace std::string_literals;

Engine::Engine(sys::Vulkan& _vk, MeshList&& _meshList):
	m_vk(_vk) {
	
	ZoneScoped;
	
	auto ifc = m_vk.context->begin();
	auto ptc = ifc.begin();
	m_permPool.setPtc(ptc);
	
	// Compile pipelines
	
	CubeFilter::compile(ptc);
	Atmosphere::compile(ptc);
	Visibility::compile(ptc);
	Indirect::compile(ptc);
	Forward::compile(ptc);
	Tonemap::compile(ptc);
	Bloom::compile(ptc);
	Sky::compile(ptc);
	
	// Initialize internal resources
	m_swapchainDirty = false;
	
	m_imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	// Begin imgui frame so that first-frame calls succeed
	ImGui::NewFrame();
	
	m_meshes.emplace(std::move(_meshList).upload(m_permPool, "Meshes"));
	m_atmosphere.upload(m_permPool, "Earth", Atmosphere::Params::earth());
	m_objects = ObjectPool();
	
	// Perform precalculations
	auto precalc = vuk::RenderGraph();
	m_atmosphere.precalculate(precalc);
	
	// Finalize
	
	ptc.wait_all_transfers();
	
	auto erg = std::move(precalc).link(ptc);
	vuk::execute_submit_and_wait(ptc, std::move(erg));
	
	L_INFO("Graphics engine initialized");
	
}

Engine::~Engine() {
	
	ZoneScoped;
	
	m_vk.context->wait_idle();
	
	m_meshes.reset();
	m_objects.reset();
	m_imguiData.fontTex.view.reset();
	m_imguiData.fontTex.image.reset();
	
	L_INFO("Graphics engine cleaned up");
	
}

void Engine::render() {
	
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
	m_permPool.setPtc(ptc);
	m_framePool.setPtc(ptc);
	auto rg = vuk::RenderGraph();
	
	// Create per-frame resources
	
	auto iblUnfiltered = Cubemap::make(m_permPool, "ibl_unfiltered",
		256, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferSrc);
	auto iblFiltered = Cubemap::make(m_permPool, "ibl_filtered",
		256, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferDst);
	iblUnfiltered.attach(rg, vuk::eNone, vuk::eNone);
	iblFiltered.attach(rg, vuk::eNone, vuk::eNone);
	
	auto depth = Texture2D::make(m_framePool, "depth",
		viewport, vuk::Format::eD32Sfloat,
		vuk::ImageUsageFlagBits::eDepthStencilAttachment);
	auto color = Texture2D::make(m_framePool, "color",
		viewport, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eColorAttachment |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eStorage);
	depth.attach(rg, vuk::eClear, vuk::eNone, vuk::ClearDepthStencil(0.0f, 0));
	color.attach(rg, vuk::eNone, vuk::eNone);
	
	auto screen = Texture2D::make(m_framePool, "screen",
		viewport, vuk::Format::eR8G8B8A8Unorm,
		vuk::ImageUsageFlagBits::eTransferSrc |
		vuk::ImageUsageFlagBits::eColorAttachment |
		vuk::ImageUsageFlagBits::eStorage);
	screen.attach(rg, vuk::eNone, vuk::eNone);
	
	auto visbuf = Texture2D::make(m_framePool, "visbuf",
		viewport, vuk::Format::eR32Uint,
		vuk::ImageUsageFlagBits::eColorAttachment);
	visbuf.attach(rg, vuk::eClear, vuk::eNone, vuk::ClearColor(-1u, 0, 0, 0));
	
	auto worldBuf = m_world.upload(m_framePool, "world");
	
	// Initialize effects
	
	auto indirect = Indirect(m_framePool, "Indirect", *m_objects, *m_meshes);
	auto sky = Sky(m_permPool, "sky", m_atmosphere);
	
	// Set up the rendergraph
	
	sky.calculate(rg, worldBuf, m_camera);
	sky.drawCubemap(rg, worldBuf, iblUnfiltered);
	CubeFilter::apply(rg, iblUnfiltered, iblFiltered);
	indirect.sortAndCull(rg, m_world, *m_meshes);
//	Forward::zPrepass(rg, depth, worldBuf, indirect, *m_meshes);
	Visibility::apply(rg, visbuf, depth, worldBuf, indirect, *m_meshes);
	Forward::draw(rg, color, depth, worldBuf, indirect, *m_meshes, sky, iblFiltered);
	sky.draw(rg, worldBuf, color, depth);
	Bloom::apply(rg, m_framePool, color);
	Tonemap::apply(rg, color, screen);
	
	ImGui::Render();
	ImGui_ImplVuk_Render(m_framePool, ptc, rg, screen.name, m_imguiData, ImGui::GetDrawData());
	
	rg.attach_swapchain("swapchain", m_vk.swapchain, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	rg.add_pass({
		.name = "swapchain copy",
		.resources = {
			screen.resource(vuk::eTransferSrc),
			"swapchain"_image(vuk::eTransferDst) },
		.execute = [screen](vuk::CommandBuffer& cmd) {
			
			cmd.blit_image(screen.name, "swapchain", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D(), vuk::Offset3D{i32(screen.size().x()), i32(screen.size().y()), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D(), vuk::Offset3D{i32(screen.size().x()), i32(screen.size().y()), 1}},
			}, vuk::Filter::eNearest);
			
		}});
	
#ifdef TRACY_ENABLE
	rg.add_tracy_collection();
#endif //TRACY_ENABLE
	
	// Acquire swapchain image
	
	auto presentSem = ptc.acquire_semaphore();
	auto swapchainImageIndex = u32(0);
	{
		
		ZoneScopedN("Acquire");
		
		auto error = vkAcquireNextImageKHR(m_vk.device.device, m_vk.swapchain->swapchain,
			UINT64_MAX, presentSem, VK_NULL_HANDLE, &swapchainImageIndex);
		if (error == VK_ERROR_OUT_OF_DATE_KHR) {
			
			m_swapchainDirty = true;
			return; // Requires repaint
			
		}
		if (error != VK_SUCCESS && error != VK_SUBOPTIMAL_KHR) // Unknown result
			throw runtime_error_fmt("Unable to acquire swapchain image: error {}", error);
		
	}
	
	// Build and submit the rendergraph
	
	auto erg = std::move(rg).link(ptc, false);
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
	
	{
		
		ZoneScopedN("Present");
		
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
		
	}
	
	// Clean up
	
	ImGui::NewFrame();
	m_framePool.reset();
	FrameMark;
	
}

void Engine::refreshSwapchain(uvec2 _newSize) {
	
	auto lock = std::lock_guard(m_renderLock);
	
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
