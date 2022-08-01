#include "gfx/renderer.hpp"

#include "config.hpp"

#include "imgui.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "util/error.hpp"
#include "util/math.hpp"
#include "util/log.hpp"
#include "sys/system.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/util.hpp"

namespace minote {

using namespace std::string_literals;

Renderer::Renderer():
	m_deviceResource(*s_vulkan->context, InflightFrames),
	m_swapchainDirty(false),
	m_flushTemporalResources(true),
	m_framerate(60.0f),
	m_lastFramerateCheck(0),
	m_framesSinceLastCheck(0),
	m_globalAllocator(m_deviceResource),
	m_frameAllocator(m_deviceResource.get_next_frame()),
	m_imgui(m_globalAllocator) {
	
	L_INFO("Renderer initialized");
	
}

Renderer::~Renderer() {
	
	s_vulkan->context->wait_idle();
	L_INFO("Renderer cleaned up");
	
}

void Renderer::render() {
	
	// If repaint needed, only resizeSwapchain() can draw
	if (m_swapchainDirty) return;
	
	auto lock = std::lock_guard(m_renderLock);
	renderFrame();

}

void Renderer::renderFrame() {
	
	// Prepare next frame
	s_vulkan->context->next_frame();
	m_imgui.begin(); // Ensure that imgui calls inside this function work; usually a no-op
	auto& frameResource = m_deviceResource.get_next_frame();
	m_frameAllocator = vuk::Allocator(frameResource);
	calcFramerate();
	
	// Prepare and upload world properties
	if (!m_flushTemporalResources)
		m_world.prevViewProjection = m_world.viewProjection;
	
	auto viewport = uvec2{s_vulkan->swapchain->extent.width, s_vulkan->swapchain->extent.height};
	m_world.projection = perspective(VerticalFov, f32(viewport.x()) / f32(viewport.y()), NearPlane);
	m_world.view = m_camera.transform();
	m_world.viewProjection = m_world.projection * m_world.view;
	m_world.viewProjectionInverse = inverse(m_world.viewProjection);
	m_world.viewportSize = viewport;
	m_world.nearPlane = NearPlane;
	m_world.cameraPos = m_camera.position;
	m_world.frameCounter = s_vulkan->context->get_frame_count();
	
	if (m_flushTemporalResources)
		m_world.prevViewProjection = m_world.viewProjection;
	
	// Atmospheric properties
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
	
	auto rg = std::make_shared<vuk::RenderGraph>("initial");
	rg->attach_swapchain("swapchain", s_vulkan->swapchain);
	rg->attach_and_clear_image("screen", { .format = vuk::Format::eR8G8B8A8Unorm, .sample_count = vuk::Samples::e1 }, vuk::ClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	rg->inference_rule("screen", vuk::same_extent_as("swapchain"));
	
	auto atmosphere = Atmosphere(m_frameAllocator, Atmosphere::Params::earth());
	
	// Draw frame
	auto screenImguiFut = m_imgui.render(vuk::Future(rg, "screen"));
	auto futures = rg->split();
	rg = std::make_shared<vuk::RenderGraph>("final");
	rg->attach_in(futures);
	rg->attach_in("transmittance", atmosphere.transmittance); // For testing
	rg->attach_in("screen_imgui", screenImguiFut);
	
	// Blit frame to swapchain
	rg->add_pass({
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
			
		},
	});
	
	// Build and submit the rendergraph
	try {
		auto compiler = vuk::Compiler();
		vuk::execute_submit_and_present_to_one(m_frameAllocator, compiler.link({&rg, 1}, {}), s_vulkan->swapchain);
	} catch (vuk::PresentException& e) {
		auto error = e.code();
		if (error == VK_ERROR_OUT_OF_DATE_KHR)
			m_swapchainDirty = true; // No need to return, only cleanup is left
		else if (error != VK_SUBOPTIMAL_KHR)
			throw runtime_error_fmt("Unable to present to the screen: error {}", error);
	}
	
	// Clean up
	
	m_flushTemporalResources = false;
	// m_objects.copyTransforms();
	
}

void Renderer::refreshSwapchain(uvec2 _newSize) {
	
	auto lock = std::lock_guard(m_renderLock);
	
	auto newSwapchain = s_vulkan->context->add_swapchain(s_vulkan->createSwapchain(_newSize, s_vulkan->swapchain->swapchain));
	m_deviceResource.deallocate_image_views(s_vulkan->swapchain->image_views);
	m_deviceResource.deallocate_swapchains({&s_vulkan->swapchain->swapchain, 1});
	s_vulkan->context->remove_swapchain(s_vulkan->swapchain);
	s_vulkan->swapchain = newSwapchain;
	m_swapchainDirty = false;
	m_flushTemporalResources = true;
	
	renderFrame();
	
}

void Renderer::calcFramerate() {
	
	m_framesSinceLastCheck += 1;
	auto currentTime = s_system->getTime();
	auto timeElapsed = currentTime - m_lastFramerateCheck;
	if (timeElapsed >= FramerateUpdate) {
		auto secondsElapsed = ratio(timeElapsed, 1_s);
		m_framerate = f32(m_framesSinceLastCheck) / secondsElapsed;
		
		m_lastFramerateCheck = currentTime;
		m_framesSinceLastCheck = 0;
	}
	
	ImGui::Text("FPS: %.1f", m_framerate);
	
}

}
