#include "gfx/engine.hpp"

#include "config.hpp"

#include <stdexcept>
#include <cstring>
#include <cassert>
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include "fmt/core.h"
#include "volk.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/modules/indirect.hpp"
#include "gfx/modules/forward.hpp"
#include "gfx/modules/bloom.hpp"
#include "gfx/modules/post.hpp"
#include "gfx/base.hpp"
#include "main.hpp"

namespace minote::gfx {

using namespace base;
using namespace std::string_literals;

#if VK_VALIDATION
VKAPI_ATTR auto VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
	VkDebugUtilsMessageTypeFlagsEXT typeCode,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void*) -> VkBool32 {
	assert(data);

	auto const severity = [severityCode] {
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			return Log::Level::Error;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			return Log::Level::Warn;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			return Log::Level::Info;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			return Log::Level::Debug;
		throw std::logic_error(fmt::format("Unknown Vulkan diagnostic message severity: #{}", severityCode));
	}();

	auto type = [typeCode]() {
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			return "[VulkanPerf]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			return "[VulkanSpec]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			return "[Vulkan]";
		throw std::logic_error(fmt::format("Unknown Vulkan diagnostic message type: #{}", typeCode));
	}();

	L.log(severity, "{} {}", type, data->pMessage);

	return VK_FALSE;
}
#endif //VK_VALIDATION

Engine::Engine(sys::Window& window, Version version) {
	// Create instance
	auto instanceResult = vkb::InstanceBuilder()
#if VK_VALIDATION
		.enable_layer("VK_LAYER_KHRONOS_validation")
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
		.set_debug_callback(debugCallback)
		.set_debug_messenger_severity(
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		.set_debug_messenger_type(
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
#endif //VK_VALIDATION
		.set_app_name(AppTitle)
		.set_engine_name("vuk")
		.require_api_version(1, 2, 0)
		.set_app_version(std::get<0>(version), std::get<1>(version), std::get<2>(version))
		.build();
	if (!instanceResult)
		throw std::runtime_error(fmt::format("Failed to create a Vulkan instance: {}", instanceResult.error().message()));
	instance = instanceResult.value();
	volkInitializeCustom(instance.fp_vkGetInstanceProcAddr);
	volkLoadInstanceOnly(instance.instance);
	L.debug("Vulkan instance created");

	// Create surface
	glfwCreateWindowSurface(instance.instance, window.handle(), nullptr, &surface);

	// Select physical device
	auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
		.multiDrawIndirect = VK_TRUE,
	};
	auto physicalDeviceVulkan12Features = VkPhysicalDeviceVulkan12Features{
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.hostQueryReset = VK_TRUE,
	};
	auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(instance)
		.set_surface(surface)
		.set_minimum_version(1, 2)
		.set_required_features(physicalDeviceFeatures)
		.set_required_features_12(physicalDeviceVulkan12Features)
#if VK_VALIDATION
		.add_required_extension("VK_KHR_shader_non_semantic_info")
#endif //VK_VALIDATION
		.select();
	if (!physicalDeviceSelectorResult)
		throw std::runtime_error(fmt::format("Failed to find a suitable GPU for Vulkan: {}", physicalDeviceSelectorResult.error().message()));
	auto physicalDevice = physicalDeviceSelectorResult.value();

	// Create device
	auto deviceResult = vkb::DeviceBuilder(physicalDevice).build();
	if (!deviceResult)
		throw std::runtime_error(fmt::format("Failed to create Vulkan device: {}", deviceResult.error().message()));
	device = deviceResult.value();
	volkLoadDevice(device.device);
	L.debug("Vulkan device created");

	// Get queues
	auto graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	auto graphicsQueueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();
	auto transferQueuePresent = device.get_dedicated_queue(vkb::QueueType::present).has_value();
	auto transferQueue = transferQueuePresent? device.get_dedicated_queue(vkb::QueueType::present).value() : VK_NULL_HANDLE;
	auto transferQueueFamilyIndex = transferQueuePresent? device.get_dedicated_queue_index(vkb::QueueType::present).value() : VK_QUEUE_FAMILY_IGNORED;

	// Create vuk context
	context.emplace(vuk::ContextCreateParameters{
		instance.instance, device.device, physicalDevice.physical_device,
		graphicsQueue, graphicsQueueFamilyIndex,
		transferQueue, transferQueueFamilyIndex
	});
	
	// Create swapchain
	swapchain = context->add_swapchain(createSwapchain());
	
	// Create user-facing modules
	meshes = modules::Meshes();
}

Engine::~Engine() {
	context->wait_idle();
	ibl.reset();
	atmosphere.reset();
	meshes.reset();
#if IMGUI
	imguiData.font_texture.view.reset();
	imguiData.font_texture.image.reset();
#endif //IMGUI
	// This performs cleanups for all inflight frames
	for (auto i = 0u; i < vuk::Context::FC; i++)
		context->begin();
	context.reset();
	vkb::destroy_device(device);
	vkDestroySurfaceKHR(instance.instance, surface, nullptr);
	vkb::destroy_instance(instance);
}

void Engine::uploadAssets() {
	auto ifc = context->begin();
	auto ptc = ifc.begin();
	
#if IMGUI
	imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(f32(swapchain->extent.width), f32(swapchain->extent.height));
#endif //IMGUI
	
	// Upload static data
	meshes->upload(ptc);
	
	// Create pipeline components
	atmosphere = modules::Atmosphere(ptc, modules::Atmosphere::Params::earth());
	ibl = modules::IBLMap(ptc);
	
	// Finalize uploads
	ptc.wait_all_transfers();
	
	// Perform precalculations
	auto precalc = atmosphere->precalculate();
	auto erg = std::move(precalc).link(ptc);
	vuk::execute_submit_and_wait(ptc, std::move(erg));
	
	// Begin imgui frame so that first-frame calls succeed
#if IMGUI
	ImGui::NewFrame();
#endif //IMGUI
}

void Engine::render() {
	
	// Prepare per-frame data
	
	auto viewport = uvec2(swapchain->extent.width, swapchain->extent.height);
	auto rawview = camera.transform();
	// auto zFlip = make_scale({-1.0f, -1.0f, 1.0f});
	world.projection = infinitePerspective(VerticalFov, f32(viewport.x) / f32(viewport.y), NearPlane);
	// Reverse-Z
	world.projection[2][2] = 0.0f;
	world.projection[3][2] *= -1.0f;
	// world.view = zFlip * rawview;
	world.view = rawview;
	world.viewProjection = world.projection * world.view;
	world.viewProjectionInverse = inverse(world.viewProjection);
	world.viewportSize = {swapchain->extent.width, swapchain->extent.height};
	world.cameraPos = camera.position;
	auto swapchainSize = vuk::Dimension2D::absolute(swapchain->extent);
	
	static auto sunPitch = 7.2_deg;
	static auto sunYaw = 30.0_deg;
#if IMGUI
	ImGui::SliderAngle("Sun pitch", &sunPitch, -8.0f, 60.0f, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderAngle("Sun yaw", &sunYaw, -180.0f, 180.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
#endif //IMGUI
	// sunPitch = radians(6.0f - glfwGetTime() / 2.0);
	world.sunDirection = vec3(1.0f, 0.0f, 0.0f);
	world.sunDirection = glm::mat3(make_rotate(sunPitch, {0.0f, -1.0f, 0.0f})) * world.sunDirection;
	world.sunDirection = glm::mat3(make_rotate(sunYaw, {0.0f, 0.0f, 1.0f})) * world.sunDirection;
	static auto sunIlluminance = 8.0f;
#if IMGUI
	ImGui::SliderFloat("Sun illuminance", &sunIlluminance, 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
#endif //IMGUI
	world.sunIlluminance = vec3(sunIlluminance);
	
	// Begin draw
	
	auto ifc = context->begin();
	auto ptc = ifc.begin();
	
	// Initialize modules
	
	auto worldBuf = world.upload(ptc);
	auto indirect = modules::Indirect(ptc, objects, *meshes);
	auto sky = modules::Sky(ptc, *atmosphere);
	auto forward = modules::Forward(ptc, swapchainSize.extent);
	auto post = modules::Post(ptc);
	auto bloom = modules::Bloom(ptc, forward.size);
	
	// Set up the rendergraph
	
	auto rg = vuk::RenderGraph();
	
	rg.append(sky.calculate(worldBuf, camera));
	rg.append(sky.drawCubemap(worldBuf, ibl->Unfiltered_n, {ibl->BaseSize, ibl->BaseSize}));
	rg.append(ibl->filter());
	rg.append(indirect.frustumCull(world));
	rg.append(forward.zPrepass(worldBuf, indirect, *meshes));
	rg.append(forward.draw(worldBuf, indirect, *meshes, sky, *ibl));
	rg.append(sky.draw(worldBuf, forward.Color_n, forward.Depth_n, swapchainSize.extent));
	rg.append(forward.resolve());
	rg.append(bloom.apply(forward.Resolved_n));
	rg.append(post.tonemap(forward.Resolved_n, "swapchain", swapchainSize.extent));
	
#if IMGUI
	ImGui::Render();
	ImGui_ImplVuk_Render(ptc, rg, "swapchain", "swapchain", imguiData, ImGui::GetDrawData());
#endif //IMGUI
	
	rg.attach_swapchain("swapchain", swapchain, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	auto erg = std::move(rg).link(ptc);
	
	// Acquire swapchain image
	
	auto presentSem = ptc.acquire_semaphore();
	auto swapchainImageIndex = [&, this] {
		while (true) {
			u32 result;
			VkResult error = vkAcquireNextImageKHR(device.device, swapchain->swapchain, UINT64_MAX, presentSem, VK_NULL_HANDLE, &result);
			if (error == VK_SUCCESS || error == VK_SUBOPTIMAL_KHR)
				return result;
			if (error == VK_ERROR_OUT_OF_DATE_KHR)
				refreshSwapchain();
			else
				throw std::runtime_error(fmt::format("Unable to acquire swapchain image: error {}", error));
		}
	}();
	
	// Build and submit the rendergraph
	
	auto commandBuffer = erg.execute(ptc, {{swapchain, swapchainImageIndex}});
	
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
		.pSignalSemaphores = &renderSem,
	};
	context->submit_graphics(submitInfo, ptc.acquire_fence());
	
	// Present to screen
	
	auto presentInfo = VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderSem,
		.swapchainCount = 1,
		.pSwapchains = &swapchain->swapchain,
		.pImageIndices = &swapchainImageIndex,
	};
	auto result = vkQueuePresentKHR(context->graphics_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		refreshSwapchain();
	else if (result != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Unable to present to the screen: error {}", result));
	
	// Clean up
	
#if IMGUI
	ImGui::NewFrame();
#endif //IMGUI
	
}

auto Engine::createSwapchain(VkSwapchainKHR old) -> vuk::Swapchain {
	auto vkbswapchainResult = vkb::SwapchainBuilder(device)
		.set_old_swapchain(old)
		.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
	if (!vkbswapchainResult)
		throw std::runtime_error(fmt::format("Failed to create the swapchain: {}", vkbswapchainResult.error().message()));
	auto vkbswapchain = vkbswapchainResult.value();

	auto vuksw = vuk::Swapchain{
		.swapchain = vkbswapchain.swapchain,
		.surface = surface,
		.format = vuk::Format(vkbswapchain.image_format),
		.extent = { vkbswapchain.extent.width, vkbswapchain.extent.height },
	};
	auto imgs = vkbswapchain.get_images();
	auto ivs = vkbswapchain.get_image_views();
	for (auto& img: *imgs)
		vuksw.images.emplace_back(img);
	for (auto& iv: *ivs)
		vuksw.image_views.emplace_back().payload = iv;
	return vuksw;
}

void Engine::refreshSwapchain() {
	for (auto iv: swapchain->image_views)
		context->enqueue_destroy(iv);
	auto newSwapchain = context->add_swapchain(createSwapchain(swapchain->swapchain));
	context->remove_swapchain(swapchain);
	swapchain = newSwapchain;
#if IMGUI
	ImGui::GetIO().DisplaySize = ImVec2(f32(swapchain->extent.width), f32(swapchain->extent.height));
#endif //IMGUI
}

}
