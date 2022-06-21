#include "sys/vulkan.hpp"

#include "config.hpp"

#include <cassert>
#include "SDL_vulkan.h"
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
	
	// Create instance
	
	auto instanceResult = vkb::InstanceBuilder()
#if VK_VALIDATION
		.enable_layer("VK_LAYER_KHRONOS_validation")
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
		.set_debug_callback(debugCallback)
		.set_debug_messenger_severity(
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT/* |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT*/)
		.set_debug_messenger_type(
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
#endif //VK_VALIDATION
		.set_app_name(AppTitle)
		.set_engine_name("vuk")
		.require_api_version(1, 3, 0)
		.set_app_version(std::get<0>(AppVersion), std::get<1>(AppVersion), std::get<2>(AppVersion))
		.build();
	if (!instanceResult)
		throw runtime_error_fmt("Failed to create a Vulkan instance: {}", instanceResult.error().message());
	instance = instanceResult.value();
	
	volkInitializeCustom(instance.fp_vkGetInstanceProcAddr);
	volkLoadInstanceOnly(instance.instance);
	
	L_DEBUG("Vulkan instance created");
	
	// Create surface
	
	SDL_Vulkan_CreateSurface(_window.handle(), instance.instance, &surface);
	
	// Select physical device
	
	auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
#if VK_VALIDATION
		.robustBufferAccess = VK_TRUE,
#endif //VK_VALIDATION
		.geometryShader = VK_TRUE, // gl_PrimitiveID requirement
		.shaderStorageImageWriteWithoutFormat = VK_TRUE };
	auto physicalDeviceVulkan11Features = VkPhysicalDeviceVulkan11Features{
		.shaderDrawParameters = VK_TRUE };
	auto physicalDeviceVulkan12Features = VkPhysicalDeviceVulkan12Features{
		.samplerFilterMinmax = VK_TRUE,
		.hostQueryReset = VK_TRUE, // vuk requirement
		.timelineSemaphore = VK_TRUE, // vuk requirement
		.vulkanMemoryModel = VK_TRUE, // general performance improvement
		.vulkanMemoryModelDeviceScope = VK_TRUE }; // general performance improvement
	auto physicalDeviceVulkan13Features = VkPhysicalDeviceVulkan13Features{
		.computeFullSubgroups = VK_TRUE,
		.synchronization2 = VK_TRUE // pending vuk bugfix
		};
	
	auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(instance)
		.set_surface(surface)
		.set_minimum_version(1, 3)
		.set_required_features(physicalDeviceFeatures)
		.set_required_features_11(physicalDeviceVulkan11Features)
		.set_required_features_12(physicalDeviceVulkan12Features)
		.set_required_features_13(physicalDeviceVulkan13Features)
		.add_required_extension("VK_KHR_synchronization2")
#if VK_VALIDATION
		.add_required_extension("VK_EXT_robustness2")
		.add_required_extension_features(VkPhysicalDeviceRobustness2FeaturesEXT{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
			.robustBufferAccess2 = VK_TRUE,
			.robustImageAccess2 = VK_TRUE })
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
	
	auto transferQueuePresent = device.get_dedicated_queue(vkb::QueueType::transfer).has_value();
	auto transferQueue = transferQueuePresent?
		device.get_dedicated_queue(vkb::QueueType::transfer).value() :
		VK_NULL_HANDLE;
	auto transferQueueFamilyIndex = transferQueuePresent?
		device.get_dedicated_queue_index(vkb::QueueType::transfer).value() :
		VK_QUEUE_FAMILY_IGNORED;
	
	auto computeQueuePresent = device.get_dedicated_queue(vkb::QueueType::compute).has_value();
	auto computeQueue = computeQueuePresent?
		device.get_dedicated_queue(vkb::QueueType::compute).value() :
		VK_NULL_HANDLE;
	auto computeQueueFamilyIndex = computeQueuePresent?
		device.get_dedicated_queue_index(vkb::QueueType::compute).value() :
		VK_QUEUE_FAMILY_IGNORED;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physicalDevice.physical_device, &props);
	
	// Create vuk context
	
	context.emplace(vuk::ContextCreateParameters{
		.instance =                    instance.instance,
		.device =                      device.device,
		.physical_device =             physicalDevice.physical_device,
		.graphics_queue =              graphicsQueue,
		.graphics_queue_family_index = graphicsQueueFamilyIndex,
		.compute_queue =               computeQueue,
		.compute_queue_family_index =  computeQueueFamilyIndex,
		.transfer_queue =              transferQueue,
		.transfer_queue_family_index = transferQueueFamilyIndex});
	
	// Create swapchain
	
	swapchain = context->add_swapchain(createSwapchain(_window.size()));
	
	L_INFO("Vulkan initialized");
	
}

Vulkan::~Vulkan() {
	
	// Await GPU idle
	
	context->wait_idle();
	
	// Cleanup vuk
	
	// This performs cleanup for all inflight frames
	// repeat(vuk::Context::FC, [this] {
	// 	context->begin();
	// });
	context.reset();
	
	// Shut down Vulkan
	
	vkb::destroy_device(device);
	vkDestroySurfaceKHR(instance.instance, surface, nullptr);
	vkb::destroy_instance(instance);
	
	L_INFO("Vulkan cleaned up");
	
}

auto Vulkan::createSwapchain(uvec2 _size, VkSwapchainKHR _old) -> vuk::Swapchain {
	
	auto vkbswapchainResult = vkb::SwapchainBuilder(device)
		.set_old_swapchain(_old)
		.set_desired_extent(_size.x(), _size.y())
		.set_desired_format({
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.add_fallback_format({
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		                       VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
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
