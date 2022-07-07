#include "gfx/engine.hpp"

#include "config.hpp"

#include <cassert>
#include "volk.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "util/error.hpp"
#include "util/math.hpp"
#include "util/log.hpp"
#include "sys/system.hpp"
// #include "gfx/effects/instanceList.hpp"
// #include "gfx/effects/quadbuffer.hpp"
// #include "gfx/effects/cubeFilter.hpp"
// #include "gfx/effects/visibility.hpp"
// #include "gfx/effects/tonemap.hpp"
// #include "gfx/effects/bloom.hpp"
// #include "gfx/effects/bvh.hpp"
// #include "gfx/effects/pbr.hpp"
// #include "gfx/effects/sky.hpp"
// #include "gfx/effects/hiz.hpp"
// #include "gfx/frame.hpp"
#include "gfx/util.hpp"
#include "main.hpp"

namespace minote {

using namespace std::string_literals;

Engine::~Engine() {
	
	m_vk.context->wait_idle();
	
	m_imguiData.fontTex.view.reset();
	m_imguiData.fontTex.image.reset();
	
	L_INFO("Graphics engine cleaned up");
	
}

void Engine::init(ModelList&& _modelList) {
	
	// TriangleList::compile(ptc);
	// QuadBuffer::compile(ptc);
	// CubeFilter::compile(ptc);
	// Atmosphere::compile(ptc);
	// Visibility::compile(ptc);
	// Worklist::compile(ptc);
	// Tonemap::compile(ptc);
	// Bloom::compile(ptc);
	// BVH::compile(ptc);
	// PBR::compile(ptc);
	// HiZ::compile(ptc);
	// Sky::compile(ptc);
	
	m_swapchainDirty = false;
	m_flushTemporalResources = true;
	
	m_imguiData = ImGui_ImplVuk_Init(m_globalAllocator);
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	// Begin imgui frame so that first-frame calls succeed
	ImGui::NewFrame();
	
	// m_models = std::move(_modelList).upload(m_permPool, "models");
	
	// Finalize
	
	// ptc.wait_all_transfers();
	
	L_INFO("Graphics engine initialized");
	
}

void Engine::render() {
	
	// Quit if repaint needed
	if (m_swapchainDirty) return;
	
	auto lock = std::lock_guard(m_renderLock);
	renderFrame();

}

void Engine::renderFrame() {

	m_vk.context->next_frame();
	auto& frameResource = m_deviceResource.get_next_frame();
	auto frameAllocator = vuk::Allocator(frameResource);
	
	// Framerate calculation
	
	m_framesSinceLastCheck += 1;
	auto currentTime = System::getTime();
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
	m_world.nearPlane = NearPlane;
	m_world.cameraPos = m_camera.position;
	m_world.frameCounter = m_vk.context->get_frame_count();
	
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
	
	auto rg = vuk::RenderGraph();
	rg.attach_swapchain("swapchain", m_vk.swapchain);
	rg.attach_and_clear_image("screen", { .format = vuk::Format::eR8G8B8A8Unorm, .sample_count = vuk::Samples::e1 }, vuk::ClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	rg.inference_rule("screen", vuk::same_extent_as("swapchain"));
	
	// Draw frame
	/*
	auto frame = Frame(*this, rg);
	frame.draw(screen, m_objects, m_flushTemporalResources);
	*/
	ImGui::Render();
	ImGui_ImplVuk_Render(frameAllocator, rg, "screen", "screen_imgui", m_imguiData, ImGui::GetDrawData(), {});
	
	// Blit frame to swapchain
	
	rg.add_pass({
		.name = "swapchain copy",
		.resources = {
			"screen_imgui"_image >> vuk::eTransferRead,
			"swapchain"_image >> vuk::eTransferWrite },
		.execute = [](vuk::CommandBuffer& cmd) {
			
			auto src = *cmd.get_resource_image_attachment("screen_imgui");
			auto dst = *cmd.get_resource_image_attachment("swapchain");
			
			cmd.blit_image("screen_imgui", "swapchain", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(src.extent.extent.width), i32(src.extent.extent.height), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(dst.extent.extent.width), i32(dst.extent.extent.height), 1}} },
				vuk::Filter::eNearest);
			
		}});
	
	// Build and submit the rendergraph
	
	try {
		
		execute_submit_and_present_to_one(frameAllocator, std::move(rg).link({}), m_vk.swapchain);

	} catch (vuk::PresentException& e) {

		auto error = e.code();
		if (error == VK_ERROR_OUT_OF_DATE_KHR)
			m_swapchainDirty = true; // No need to return, only cleanup is left
		else if (error != VK_SUBOPTIMAL_KHR)
			throw runtime_error_fmt("Unable to present to the screen: error {}", error);
		
	}
	
	// Clean up
	
	m_flushTemporalResources = false;
	ImGui::NewFrame();
	// m_objects.copyTransforms();
	
}

void Engine::refreshSwapchain(uvec2 _newSize) {
	
	auto lock = std::lock_guard(m_renderLock);
	
	auto newSwapchain = m_vk.context->add_swapchain(m_vk.createSwapchain(_newSize, m_vk.swapchain->swapchain));
	m_deviceResource.deallocate_image_views(m_vk.swapchain->image_views);
	m_deviceResource.deallocate_swapchains({&m_vk.swapchain->swapchain, 1});
	m_vk.context->remove_swapchain(m_vk.swapchain);
	m_vk.swapchain = newSwapchain;
	m_swapchainDirty = false;
	m_flushTemporalResources = true;
	
	ImGui::GetIO().DisplaySize = ImVec2(
		f32(m_vk.swapchain->extent.width),
		f32(m_vk.swapchain->extent.height));
	
	renderFrame();
	
}

}
