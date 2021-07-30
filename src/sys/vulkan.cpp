#include "sys/vulkan.hpp"

#include "config.hpp"

#include <cassert>
#include "optick_core.h"
#include "optick.h"
#include "GLFW/glfw3.h"
#include "base/error.hpp"
#include "base/log.hpp"
#include "main.hpp"

namespace minote::sys {

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

Vulkan::Vulkan(Window& _window) {
	
	OPTICK_EVENT("Vulkan::Vulkan");
	
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
	instance = instanceResult.value();
	
	volkInitializeCustom(instance.fp_vkGetInstanceProcAddr);
	volkLoadInstanceOnly(instance.instance);
	
	L_DEBUG("Vulkan instance created");
	
	// Create surface
	
	glfwCreateWindowSurface(instance.instance, _window.handle(), nullptr, &surface);
	
	// Select physical device
	
	auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
		.multiDrawIndirect = VK_TRUE };
	auto physicalDeviceVulkan12Features = VkPhysicalDeviceVulkan12Features{
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.hostQueryReset = VK_TRUE };
	
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
	device = deviceResult.value();
	
	volkLoadDevice(device.device);
	
	L_DEBUG("Vulkan device created");
	
	// Get queues
	
	auto graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	auto graphicsQueueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();
	auto transferQueuePresent = device.get_dedicated_queue(vkb::QueueType::present).has_value();
	auto transferQueue = transferQueuePresent?
		device.get_dedicated_queue(vkb::QueueType::present).value() :
		VK_NULL_HANDLE;
	auto transferQueueFamilyIndex = transferQueuePresent?
		device.get_dedicated_queue_index(vkb::QueueType::present).value() :
		VK_QUEUE_FAMILY_IGNORED;
	
	// Create vuk context
	
	context.emplace(vuk::ContextCreateParameters{
		instance.instance, device.device, physicalDevice.physical_device,
		graphicsQueue, graphicsQueueFamilyIndex,
		transferQueue, transferQueueFamilyIndex});
	
	// Create swapchain
	
	swapchain = context->add_swapchain(createSwapchain());
	
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
		&device.device, &device.physical_device.physical_device,
		&context->graphics_queue, &context->graphics_queue_family_index, 1,
		&optickVulkanFunctions);
	
	L_INFO("Vulkan initialized");
	
}

Vulkan::~Vulkan() {
	
	OPTICK_EVENT("Vulkan::~Vulkan");
	
	// Await GPU idle
	
	context->wait_idle();
	
	// Cleanup GPU profiling
	
	Optick::Core::Get().Shutdown();
	
	// Cleanup vuk
	
	// This performs cleanup for all inflight frames
	repeat(vuk::Context::FC, [this] {
		context->begin();
	});
	context.reset();
	
	// Shut down Vulkan
	
	vkb::destroy_device(device);
	vkDestroySurfaceKHR(instance.instance, surface, nullptr);
	vkb::destroy_instance(instance);
	
	L_INFO("Vulkan cleaned up");
	
}

auto Vulkan::createSwapchain(VkSwapchainKHR _old) -> vuk::Swapchain {
	
	auto vkbswapchainResult = vkb::SwapchainBuilder(device)
		.set_old_swapchain(_old)
		.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		.build();
	if (!vkbswapchainResult)
		throw runtime_error_fmt("Failed to create the swapchain: {}", vkbswapchainResult.error().message());
	auto vkbswapchain = vkbswapchainResult.value();

	auto vuksw = vuk::Swapchain{
		.swapchain = vkbswapchain.swapchain,
		.surface = surface,
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

}
