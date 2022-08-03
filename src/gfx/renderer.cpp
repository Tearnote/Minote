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
#include "gfx/effects/tonemap.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/util.hpp"

namespace minote {

struct Renderer::Impl {
	optional<Atmosphere> m_atmosphere;
	Tonemap m_tonemap;
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
	
	beginFrame();
	auto rg = buildRenderGraph();
	submitAndPresent(rg);
	
}

void Renderer::beginFrame() {
	
	s_vulkan->context->next_frame();
	m_imgui.begin(); // Ensure that imgui calls work during rendering; usually a no-op
	auto& frameResource = m_deviceResource.get_next_frame();
	m_frameAllocator = vuk::Allocator(frameResource);
	calcFramerate();
	m_camera.viewport = uvec2{s_vulkan->swapchain->extent.width, s_vulkan->swapchain->extent.height};
	
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

auto Renderer::buildRenderGraph() -> std::shared_ptr<vuk::RenderGraph> {
	
	// Initial resources
	auto rg = std::make_shared<vuk::RenderGraph>("init");
	rg->attach_and_clear_image("screen", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(m_camera.viewport.x(), m_camera.viewport.y()),
		.format = vuk::Format::eR16G16B16A16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	}, vuk::ClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	auto screen = vuk::Future(rg, "screen");
	
	// Sky rendering
	if (!m_impl->m_atmosphere.has_value())
		m_impl->m_atmosphere.emplace(m_globalAllocator, Atmosphere::Params::earth());
	auto skyView = Sky::createView(*m_impl->m_atmosphere, m_world, m_camera.position);
	auto screenSky = Sky::draw(screen, *m_impl->m_atmosphere, skyView, m_world, m_camera);
	
	// Tonemap and convert to sRGB
	auto screenSrgb = m_impl->m_tonemap.apply(screenSky);
	
	// Imgui rendering
	auto screenFinal = m_imgui.render(screenSrgb);
	
	// Copy to swapchain
	rg = std::make_shared<vuk::RenderGraph>("main");
	rg->attach_in("screen/final", screenFinal);
	rg->attach_swapchain("swapchain", s_vulkan->swapchain);
	rg->add_pass({
		.name = "swapchain copy",
		.resources = {
			"screen/final"_image >> vuk::eTransferRead,
			"swapchain"_image >> vuk::eTransferWrite },
		.execute = [](vuk::CommandBuffer& cmd) {
			auto srcSize = cmd.get_resource_image_attachment("screen/final").value().extent.extent;
			auto dstSize = cmd.get_resource_image_attachment("swapchain").value().extent.extent;
			cmd.blit_image("screen/final", "swapchain", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(srcSize.width), i32(srcSize.height), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(dstSize.width), i32(dstSize.height), 1}} },
				vuk::Filter::eNearest);
		},
	});
	
	return rg;
	
}

void Renderer::submitAndPresent(std::shared_ptr<vuk::RenderGraph> _rg) try {
	
	auto compiler = vuk::Compiler();
	vuk::execute_submit_and_present_to_one(m_frameAllocator, compiler.link({&_rg, 1}, {}), s_vulkan->swapchain);
	
} catch (vuk::PresentException& e) {
	
	auto error = e.code();
	if (error == VK_ERROR_OUT_OF_DATE_KHR)
		m_swapchainDirty = true; // No need to return, only cleanup is left
	else if (error != VK_SUBOPTIMAL_KHR)
		throw runtime_error_fmt("Unable to present to the screen: error {}", error);
	
}

}
