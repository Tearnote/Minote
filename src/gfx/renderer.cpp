#include "gfx/renderer.hpp"

#include "config.hpp"

#include <optional>
#include "imgui.h"
#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "util/error.hpp"
#include "util/math.hpp"
#include "util/log.hpp"
#include "sys/system.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/util.hpp"

namespace minote {

struct Renderer::Impl {
	optional<Atmosphere> m_atmosphere;
};

Renderer::Renderer():
	m_deviceResource(*s_vulkan->context, InflightFrames),
	m_swapchainDirty(false),
	m_framerate(60.0f),
	m_lastFramerateCheck(0),
	m_framesSinceLastCheck(0),
	m_globalAllocator(m_deviceResource),
	m_frameAllocator(m_deviceResource.get_next_frame()),
	m_imgui(m_globalAllocator),
	m_impl(new Impl()) {
	
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
	m_camera.viewport = uvec2{s_vulkan->swapchain->extent.width, s_vulkan->swapchain->extent.height};
	
	// Build rendergraph
	auto rg = std::make_shared<vuk::RenderGraph>("initial");
	rg->attach_swapchain("swapchain", s_vulkan->swapchain);
	rg->attach_and_clear_image("screen", { .format = vuk::Format::eR8G8B8A8Unorm, .sample_count = vuk::Samples::e1 }, vuk::ClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	rg->inference_rule("screen", vuk::same_extent_as("swapchain"));
	
	if (!m_impl->m_atmosphere.has_value())
		m_impl->m_atmosphere.emplace(m_globalAllocator, Atmosphere::Params::earth());
	auto skyView = Sky::createView(*m_impl->m_atmosphere, m_world, m_camera.position);
	auto screenSky = Sky::draw(vuk::Future(rg, "screen"), *m_impl->m_atmosphere, skyView, m_world, m_camera);
	
	auto screenImguiFut = m_imgui.render(screenSky);
	auto futures = rg->split();
	rg = std::make_shared<vuk::RenderGraph>("main");
	rg->attach_in(futures);
	rg->attach_in("screen_imgui", screenImguiFut);
	
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
	
	// Build and submit rendergraph
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
