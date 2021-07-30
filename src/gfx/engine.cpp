#include "gfx/engine.hpp"

#include "config.hpp"

#include <cassert>
#include "optick_core.h"
#include "optick.h"
#include "GLFW/glfw3.h"
#include "volk.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "base/error.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/module/indirect.hpp"
#include "gfx/module/tonemap.hpp"
#include "gfx/module/forward.hpp"
#include "gfx/module/bloom.hpp"
#include "gfx/base.hpp"
#include "main.hpp"

namespace minote::gfx {

using namespace base;
using namespace std::string_literals;

#if VK_VALIDATION
VKAPI_ATTR auto VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT _severityCode,
	VkDebugUtilsMessageTypeFlagsEXT _typeCode,
	VkDebugUtilsMessengerCallbackDataEXT const* _data,
	void*) -> VkBool32 {
	
	assert(_data);
	
	auto type = [_typeCode]() {
		
		if (_typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			return "[VulkanPerf]";
		if (_typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			return "[VulkanSpec]";
		if (_typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			return "[Vulkan]";
		throw logic_error_fmt("Unknown Vulkan diagnostic message type: #{}", _typeCode);
		
	}();
	
	     if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		L_ERROR("{} {}", type, _data->pMessage);
	else if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		L_WARN("{} {}", type, _data->pMessage);
	else if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		L_INFO("{} {}", type, _data->pMessage);
	else if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		L_DEBUG("{} {}", type, _data->pMessage);
	else
		throw logic_error_fmt("Unknown Vulkan diagnostic message severity: #{}", _severityCode);
	
	return VK_FALSE;
	
}
#endif //VK_VALIDATION

Engine::Engine(sys::Window& _window, MeshList&& _meshList) {
	
	OPTICK_EVENT("Engine::Engine");
	
	// Create instance
	
	auto instanceResult = vkb::InstanceBuilder()
#if VK_VALIDATION
		.enable_layer("VK_LAYER_KHRONOS_validation")
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
		.set_debug_callback(debugCallback)
		.set_debug_messenger_severity(
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT /*|
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT*/)
		.set_debug_messenger_type(
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
#endif //VK_VALIDATION
		.set_app_name(AppTitle)
		.set_engine_name("vuk")
		.require_api_version(1, 2, 0)
		.set_app_version(std::get<0>(AppVersion), std::get<1>(AppVersion), std::get<2>(AppVersion))
		.build();
	if (!instanceResult)
		throw runtime_error_fmt("Failed to create a Vulkan instance: {}", instanceResult.error().message());
	m_instance = instanceResult.value();
	
	volkInitializeCustom(m_instance.fp_vkGetInstanceProcAddr);
	volkLoadInstanceOnly(m_instance.instance);
	
	L_DEBUG("Vulkan instance created");
	
	// Create surface
	
	glfwCreateWindowSurface(m_instance.instance, _window.handle(), nullptr, &m_surface);
	
	// Select physical device
	
	auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
		.multiDrawIndirect = VK_TRUE };
	auto physicalDeviceVulkan12Features = VkPhysicalDeviceVulkan12Features{
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.hostQueryReset = VK_TRUE };
	
	auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(m_instance)
		.set_surface(m_surface)
		.set_minimum_version(1, 2)
		.set_required_features(physicalDeviceFeatures)
		.set_required_features_12(physicalDeviceVulkan12Features)
#if VK_VALIDATION
		.add_required_extension("VK_KHR_shader_non_semantic_info")
#endif //VK_VALIDATION
		.select();
	if (!physicalDeviceSelectorResult)
		throw runtime_error_fmt("Failed to find a suitable GPU for Vulkan: {}",
			physicalDeviceSelectorResult.error().message());
	auto physicalDevice = physicalDeviceSelectorResult.value();
	
	L_INFO("GPU selected: {}", physicalDevice.properties.deviceName);
	L_DEBUG("Vulkan driver version {}.{}.{}",
		VK_API_VERSION_MAJOR(physicalDevice.properties.driverVersion),
		VK_API_VERSION_MINOR(physicalDevice.properties.driverVersion),
		VK_API_VERSION_PATCH(physicalDevice.properties.driverVersion));
	
	// Create device
	
	auto deviceResult = vkb::DeviceBuilder(physicalDevice).build();
	if (!deviceResult)
		throw runtime_error_fmt("Failed to create Vulkan device: {}", deviceResult.error().message());
	m_device = deviceResult.value();
	
	volkLoadDevice(m_device.device);
	
	L_DEBUG("Vulkan device created");
	
	// Get queues
	
	auto graphicsQueue = m_device.get_queue(vkb::QueueType::graphics).value();
	auto graphicsQueueFamilyIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();
	auto transferQueuePresent = m_device.get_dedicated_queue(vkb::QueueType::present).has_value();
	auto transferQueue = transferQueuePresent?
		m_device.get_dedicated_queue(vkb::QueueType::present).value() :
		VK_NULL_HANDLE;
	auto transferQueueFamilyIndex = transferQueuePresent?
		m_device.get_dedicated_queue_index(vkb::QueueType::present).value() :
		VK_QUEUE_FAMILY_IGNORED;
	
	// Create vuk context
	
	m_context.emplace(vuk::ContextCreateParameters{
		m_instance.instance, m_device.device, physicalDevice.physical_device,
		graphicsQueue, graphicsQueueFamilyIndex,
		transferQueue, transferQueueFamilyIndex});
	
	// Create swapchain
	
	m_swapchain = m_context->add_swapchain(createSwapchain());
	
	// Initialize profiling
	
	auto optickVulkanFunctions = Optick::VulkanFunctions{
		PFN_vkGetPhysicalDeviceProperties_(vkGetPhysicalDeviceProperties),
		PFN_vkCreateQueryPool_(vkCreateQueryPool),
		PFN_vkCreateCommandPool_(vkCreateCommandPool),
		PFN_vkAllocateCommandBuffers_(vkAllocateCommandBuffers),
		PFN_vkCreateFence_(vkCreateFence),
		PFN_vkCmdResetQueryPool_(vkCmdResetQueryPool),
		PFN_vkQueueSubmit_(vkQueueSubmit),
		PFN_vkWaitForFences_(vkWaitForFences),
		PFN_vkResetCommandBuffer_(vkResetCommandBuffer),
		PFN_vkCmdWriteTimestamp_(vkCmdWriteTimestamp),
		PFN_vkGetQueryPoolResults_(vkGetQueryPoolResults),
		PFN_vkBeginCommandBuffer_(vkBeginCommandBuffer),
		PFN_vkEndCommandBuffer_(vkEndCommandBuffer),
		PFN_vkResetFences_(vkResetFences),
		PFN_vkDestroyCommandPool_(vkDestroyCommandPool),
		PFN_vkDestroyQueryPool_(vkDestroyQueryPool),
		PFN_vkDestroyFence_(vkDestroyFence),
		PFN_vkFreeCommandBuffers_(vkFreeCommandBuffers) };
	OPTICK_GPU_INIT_VULKAN(
		&m_device.device, &m_device.physical_device.physical_device,
		&m_context->graphics_queue, &m_context->graphics_queue_family_index, 1,
		&optickVulkanFunctions);
	
	// Initialize internal systems
	
	auto ifc = m_context->begin();
	auto ptc = ifc.begin();
	
	m_imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(f32(m_swapchain->extent.width), f32(m_swapchain->extent.height));
	// Begin imgui frame so that first-frame calls succeed
	ImGui::NewFrame();
	
	// Initialize persistent resources
	
	m_meshes = std::move(_meshList).upload(ptc);
	m_atmosphere = Atmosphere(ptc, Atmosphere::Params::earth());
	m_ibl = IBLMap(ptc);
	
	// Perform precalculations
	auto precalc = m_atmosphere->precalculate();
	
	// Initialize user components
	
	m_objects.emplace(*m_meshes);
	
	// Finalize
	
	ptc.wait_all_transfers();
	
	auto erg = std::move(precalc).link(ptc);
	vuk::execute_submit_and_wait(ptc, std::move(erg));
	
	L_INFO("Graphics engine initialized");
	
}

Engine::~Engine() {
	
	OPTICK_EVENT("Engine::~Engine");
	
	// Await GPU idle
	
	m_context->wait_idle();
	
	// Destroy direct resources
	
	m_ibl.reset();
	m_atmosphere.reset();
	m_objects.reset();
	m_meshes.reset();
	
	// Cleanup internal systems
	
	m_imguiData.fontTex.view.reset();
	m_imguiData.fontTex.image.reset();
	Optick::Core::Get().Shutdown();
	
	// Cleanup vuk
	
	// This performs cleanup for all inflight frames
	repeat(vuk::Context::FC, [this] {
		m_context->begin();
	});
	m_context.reset();
	
	// Shut down Vulkan
	
	vkb::destroy_device(m_device);
	vkDestroySurfaceKHR(m_instance.instance, m_surface, nullptr);
	vkb::destroy_instance(m_instance);
	
	L_INFO("Graphics engine cleaned up");
	
}

void Engine::render() {
	
	OPTICK_EVENT("Engine::render");
	
	// Prepare per-frame data
	
	// Basic scene properties
	auto viewport = uvec2{m_swapchain->extent.width, m_swapchain->extent.height};
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
	// sunPitch = radians(6.0f - glfwGetTime() / 2.0);
	m_world.sunDirection =
		mat3::rotate({0.0f, 0.0f, 1.0f}, sunYaw) *
		mat3::rotate({0.0f, -1.0f, 0.0f}, sunPitch) *
		vec3{1.0f, 0.0f, 0.0f};
	
	static auto sunIlluminance = 4.0f;
	ImGui::SliderFloat("Sun illuminance", &sunIlluminance, 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
	m_world.sunIlluminance = vec3(sunIlluminance);
	
	// Begin frame
	
	auto ifc = m_context->begin();
	auto ptc = ifc.begin();
	
	// Initialize modules
	
	auto worldBuf = m_world.upload(ptc);
	auto indirect = Indirect(ptc, *m_objects, *m_meshes);
	auto sky = Sky(ptc, *m_atmosphere);
	auto forward = Forward(ptc, viewport);
	auto tonemap = Tonemap(ptc);
	auto bloom = Bloom(ptc, forward.size());
	
	// Set up the rendergraph
	
	auto rg = vuk::RenderGraph();
	
	rg.append(sky.calculate(worldBuf, m_camera));
	rg.append(sky.drawCubemap(worldBuf, m_ibl->Unfiltered_n, uvec2(m_ibl->BaseSize)));
	rg.append(m_ibl->filter());
	rg.append(indirect.sortAndCull(m_world));
	rg.append(forward.zPrepass(worldBuf, indirect, *m_meshes));
	rg.append(forward.draw(worldBuf, indirect, *m_meshes, sky, *m_ibl));
	rg.append(sky.draw(worldBuf, forward.Color_n, forward.Depth_n, viewport));
	rg.append(bloom.apply(forward.Color_n));
	rg.append(tonemap.apply(forward.Color_n, "swapchain", viewport));
	
	ImGui::Render();
	ImGui_ImplVuk_Render(ptc, rg, "swapchain", "swapchain", m_imguiData, ImGui::GetDrawData());
	
	rg.attach_swapchain("swapchain", m_swapchain, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	
	// Acquire swapchain image
	
	auto presentSem = ptc.acquire_semaphore();
	auto swapchainImageIndex = [&, this] {
		
		while (true) {
			
			auto result = 0u;
			auto error = vkAcquireNextImageKHR(m_device.device, m_swapchain->swapchain,
				UINT64_MAX, presentSem, VK_NULL_HANDLE, &result);
			if (error == VK_SUCCESS || error == VK_SUBOPTIMAL_KHR)
				return result;
			if (error == VK_ERROR_OUT_OF_DATE_KHR)
				refreshSwapchain();
			else
				throw runtime_error_fmt("Unable to acquire swapchain image: error {}", error);
			
		}
		
	}();
	
	// Build and submit the rendergraph
	
	auto erg = std::move(rg).link(ptc);
	auto commandBuffer = erg.execute(ptc, {{m_swapchain, swapchainImageIndex}});
	
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
	m_context->submit_graphics(submitInfo, ptc.acquire_fence());
	
	// Present to screen
	
	OPTICK_GPU_FLIP(m_swapchain);
	auto presentInfo = VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderSem,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain->swapchain,
		.pImageIndices = &swapchainImageIndex};
	auto result = vkQueuePresentKHR(m_context->graphics_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		refreshSwapchain();
	else if (result != VK_SUCCESS)
		throw runtime_error_fmt("Unable to present to the screen: error {}", result);
	
	// Clean up
	
	ImGui::NewFrame();
	
}

auto Engine::createSwapchain(VkSwapchainKHR _old) -> vuk::Swapchain {
	
	auto vkbswapchainResult = vkb::SwapchainBuilder(m_device)
		.set_old_swapchain(_old)
		.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		.build();
	if (!vkbswapchainResult)
		throw runtime_error_fmt("Failed to create the swapchain: {}", vkbswapchainResult.error().message());
	auto vkbswapchain = vkbswapchainResult.value();

	auto vuksw = vuk::Swapchain{
		.swapchain = vkbswapchain.swapchain,
		.surface = m_surface,
		.format = vuk::Format(vkbswapchain.image_format),
		.extent = {vkbswapchain.extent.width, vkbswapchain.extent.height}};
	auto imgs = vkbswapchain.get_images();
	auto ivs = vkbswapchain.get_image_views();
	for (auto& img: *imgs)
		vuksw.images.emplace_back(img);
	for (auto& iv: *ivs)
		vuksw.image_views.emplace_back().payload = iv;
	return vuksw;
	
}

void Engine::refreshSwapchain() {
	
	for (auto iv: m_swapchain->image_views)
		m_context->enqueue_destroy(iv);
	
	auto newSwapchain = m_context->add_swapchain(createSwapchain(m_swapchain->swapchain));
	m_context->remove_swapchain(m_swapchain);
	m_swapchain = newSwapchain;
	
	ImGui::GetIO().DisplaySize = ImVec2(f32(m_swapchain->extent.width), f32(m_swapchain->extent.height));
	
}

}
