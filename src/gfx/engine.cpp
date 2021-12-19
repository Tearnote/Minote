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
#include "sys/system.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/effects/quadbuffer.hpp"
#include "gfx/effects/cubeFilter.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/effects/tonemap.hpp"
#include "gfx/effects/bloom.hpp"
#include "gfx/effects/pbr.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/frame.hpp"
#include "gfx/util.hpp"
#include "main.hpp"

namespace minote::gfx {

using namespace base;
using namespace std::string_literals;

static constexpr auto compileOpts = vuk::RenderGraph::CompileOptions{
	.reorder_passes = false,
#if VK_VALIDATION
	.check_pass_ordering = true,
#endif //VK_VALIDATION
};

Engine::~Engine() {
	
	ZoneScoped;
	
	m_vk.context->wait_idle();
	
	m_imguiData.fontTex.view.reset();
	m_imguiData.fontTex.image.reset();
	
	L_INFO("Graphics engine cleaned up");
	
}

void Engine::init(ModelList&& _modelList) {
	
	ZoneScoped;
	
	auto ifc = m_vk.context->begin();
	auto ptc = ifc.begin();
	m_permPool.setPtc(ptc);
	
	DrawableInstanceList::compile(ptc);
	InstanceList::compile(ptc);
	QuadBuffer::compile(ptc);
	CubeFilter::compile(ptc);
	Atmosphere::compile(ptc);
	Visibility::compile(ptc);
	Worklist::compile(ptc);
	Tonemap::compile(ptc);
	Bloom::compile(ptc);
	PBR::compile(ptc);
	Sky::compile(ptc);
	
	m_swapchainDirty = false;
	m_flushTemporalResources = true;
	
	m_imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	// Begin imgui frame so that first-frame calls succeed
	ImGui::NewFrame();
	
	m_models = std::move(_modelList).upload(m_permPool, "models");
	
	// Finalize
	
	ptc.wait_all_transfers();
	
	L_INFO("Graphics engine initialized");
	
}

void Engine::render() {
	
	ZoneScoped;
	
	// Quit if repaint needed
	if (m_swapchainDirty) return;
	
	// Lock the renderer
	m_renderLock.lock();
	defer { m_renderLock.unlock(); };
	
	// Framerate calculation
	
	m_framesSinceLastCheck += 1;
	auto currentTime = sys::System::getTime();
	auto timeElapsed = currentTime - m_lastFramerateCheck;
	if (timeElapsed >= 1_s) {
		
		auto secondsElapsed = ratio(timeElapsed, 1_s);
		m_framerate = f32(m_framesSinceLastCheck) / secondsElapsed;
		
		m_lastFramerateCheck = currentTime;
		m_framesSinceLastCheck = 0;
		
	}
	
	ImGui::Text("FPS: %.1f", m_framerate);
	
	// Prepare per-frame data
	
	// Basic scene properties
	if (!m_flushTemporalResources)
		m_world.prevViewProjection = m_world.viewProjection;
	
	auto viewport = uvec2{m_vk.swapchain->extent.width, m_vk.swapchain->extent.height};
	m_world.projection = perspective(VerticalFov, f32(viewport.x()) / f32(viewport.y()), NearPlane);
	m_world.view = m_camera.transform();
	m_world.viewProjection = m_world.projection * m_world.view;
	m_world.viewProjectionInverse = inverse(m_world.viewProjection);
	m_world.viewportSize = viewport;
	m_world.cameraPos = m_camera.position;
	m_world.frameCounter = m_vk.context->frame_counter.load();
	
	if (m_flushTemporalResources)
		m_world.prevViewProjection = m_world.viewProjection;
	
	// Sun properties
	static auto sunPitch = 16_deg;
	static auto sunYaw = 8_deg;
	ImGui::SliderAngle("Sun pitch", &sunPitch, -8.0f, 60.0f, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderAngle("Sun yaw", &sunYaw, -180.0f, 180.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	m_world.sunDirection =
		mat3::rotate({0.0f, 0.0f, 1.0f}, sunYaw) *
		mat3::rotate({0.0f, -1.0f, 0.0f}, sunPitch) *
		vec3{1.0f, 0.0f, 0.0f};
	
	static auto sunIlluminance = 4.0f;
	ImGui::SliderFloat("Sun illuminance", &sunIlluminance, 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
	m_world.sunIlluminance = vec3(sunIlluminance);
	
	// Prepare frame
	
	auto ifc = m_vk.context->begin();
	auto ptc = ifc.begin();
	m_permPool.setPtc(ptc);
	m_framePool.setPtc(ptc);
	m_swapchainPool.setPtc(ptc);
	auto rg = vuk::RenderGraph();
	
	// Create main rendering destination
	
	auto screen = Texture2D::make(m_swapchainPool, "screen",
		viewport, vuk::Format::eR8G8B8A8Unorm,
		vuk::ImageUsageFlagBits::eTransferSrc |
		vuk::ImageUsageFlagBits::eColorAttachment |
		vuk::ImageUsageFlagBits::eStorage);
	screen.attach(rg, vuk::eNone, vuk::eNone);
	
	// Draw frame
	
	auto frame = Frame(*this, rg);
	frame.draw(screen, m_objects, m_flushTemporalResources);
	
	ImGui::Render();
	ImGui_ImplVuk_Render(m_framePool, ptc, rg, screen.name, m_imguiData, ImGui::GetDrawData());
	
	// Blit frame to swapchain
	
	rg.attach_swapchain("swapchain", m_vk.swapchain, vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f));
	rg.add_pass({
		.name = "swapchain copy",
		.resources = {
			screen.resource(vuk::eTransferSrc),
			"swapchain"_image(vuk::eTransferDst) },
		.execute = [screen](vuk::CommandBuffer& cmd) {
			
			cmd.blit_image(screen.name, "swapchain", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(screen.size().x()), i32(screen.size().y()), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(screen.size().x()), i32(screen.size().y()), 1}} },
				vuk::Filter::eNearest);
			
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
	
	auto erg = std::move(rg).link(ptc, compileOpts);
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
	
	m_flushTemporalResources = false;
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
	
	m_swapchainPool.reset();
	m_flushTemporalResources = true;
	
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	
}

}
