#include "gfx/context.hpp"

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <string>
#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include "base/zip_view.hpp"
#include "base/log.hpp"
#include "sys/vk/debug.hpp"
#include "sys/vk/base.hpp"

namespace minote::gfx {

using namespace std::string_literals;
namespace ranges = std::ranges;
namespace vk = sys::vk;

static constexpr auto versionToCode(Version version) -> u32 {
	return (std::get<0>(version) << 22) + (std::get<1>(version) << 12) + std::get<2>(version);
}

static constexpr auto codeToVersion(u32 code) -> Version {
	return {
		code >> 22,
		(code >> 12) & ((1 << 10) - 1),
		code & ((1 << 12) - 1)
	};
}

void Context::init(sys::Window& _window, u32 vulkanVersion, std::string_view appName, Version appVersion) {
	window = &_window;
	name = std::string{appName};

	if (!glfwVulkanSupported())
		throw std::runtime_error("Vulkan is not supported by your system");

	// Check if the required Vulkan version is available
	volkInitializeCustom(&glfwGetInstanceProcAddress);
	L.info("Requesting Vulkan version {}", codeToVersion(vulkanVersion));
	auto const vkVersionCode = volkGetInstanceVersion();
	L.info("Vulkan version {} found", codeToVersion(vkVersionCode));
	if (vkVersionCode < vulkanVersion)
		throw std::runtime_error{"Incompatible Vulkan version"};

	// Fill in application info
	auto const appVersionCode = versionToCode(appVersion);
	auto const appNameStr = std::string{appName};
	auto const appInfo = VkApplicationInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = appNameStr.c_str(),
		.applicationVersion = appVersionCode,
		.pEngineName = "No Engine",
		.engineVersion = appVersionCode,
		.apiVersion = vulkanVersion,
	};

	// Enumerate required extensions
	u32 glfwExtensionCount;
	char const** const glfwExtensions{glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};
	ASSERT(glfwExtensions);
	instanceExtensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef VK_VALIDATION
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif //VK_VALIDATION
	L.debug("Requesting {} Vulkan instance extensions:", instanceExtensions.size());
	for (char const* e: instanceExtensions)
		L.debug("  {}", e);

	// Get available extensions
	u32 extensionCount;
	VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	auto extensions = std::vector<VkExtensionProperties>(extensionCount);
	VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));
	L.debug("Found {} Vulkan instance extensions:", extensions.size());
	for (auto const& e: extensions)
		L.debug("  {}", e.extensionName);

	// Check if all required extensions are available
	for (char const* req: instanceExtensions) {
		auto const result = ranges::find_if(extensions, [=](auto const& ext) {
			return std::strcmp(req, ext.extensionName) == 0;
		});
		if (result == extensions.end())
			throw std::runtime_error{fmt::format("Required Vulkan extension {} is not supported", req)};
	}

#ifdef VK_VALIDATION
	// Enumerate required layers
	instanceLayers = {"VK_LAYER_KHRONOS_validation"};
	L.debug("Requesting {} Vulkan layer{}:",
		instanceLayers.size(), instanceLayers.size() > 1? "s" : "");
	for (char const* l: instanceLayers)
		L.debug("  {}", l);

	// Get available layers
	u32 layerCount;
	VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
	auto layers = std::vector<VkLayerProperties>(layerCount);
	VK(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()));
	L.debug("Found {} Vulkan layers:", layers.size());
	for (auto const& l: layers)
		L.debug("  {}", l.layerName);

	// Check if all required layers are available
	for (char const* req: instanceLayers) {
		auto const result = ranges::find_if(layers, [=](auto const& l) {
			return std::strcmp(req, l.layerName) == 0;
		});
		if (result == layers.end())
			throw std::runtime_error{fmt::format("Required Vulkan layer {} is not supported", req)};
	}

	// Prepare the debug messenger
	auto const debugCI = VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = /* VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &debugCallback,
	};
#endif //VK_VALIDATION

	// Create the Vulkan instance
	auto instanceCI = VkInstanceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef VK_VALIDATION
		.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT const*>(&debugCI),
#endif //VK_VALIDATION
		.pApplicationInfo = &appInfo,
#ifdef VK_VALIDATION
		.enabledLayerCount = static_cast<u32>(instanceLayers.size()),
		.ppEnabledLayerNames = instanceLayers.data(),
#endif //VK_VALIDATION
		.enabledExtensionCount = static_cast<u32>(instanceExtensions.size()),
		.ppEnabledExtensionNames = instanceExtensions.data(),
	};
	VK(vkCreateInstance(&instanceCI, nullptr, &instance));
	volkLoadInstanceOnly(instance);
#ifdef VK_VALIDATION
	VK(vkCreateDebugUtilsMessengerEXT(instance, &debugCI, nullptr, &debugMessenger));
#endif //VK_VALIDATION
	VK(glfwCreateWindowSurface(instance, window->handle(), nullptr, &surface));

	L.debug("Vulkan instance created");

	// Enumerate required physical device extensions
	deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	L.debug("Requesting {} Vulkan device extension{}:", deviceExtensions.size(),
		deviceExtensions.size() > 1? "s" : "");
	for (char const* e: deviceExtensions)
		L.debug("  {}", e);

	// Get information on all available devices
	u32 deviceCount;
	VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	if (!deviceCount)
		throw std::runtime_error{"Failed to find any GPUs with Vulkan support"};
	auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
	auto physicalDevicesProperties = std::vector<VkPhysicalDeviceProperties>(deviceCount);
	auto physicalDevicesFeatures = std::vector<VkPhysicalDeviceFeatures>(deviceCount);
	auto physicalDevicesExtensions = std::vector<std::vector<VkExtensionProperties>>(deviceCount);
	auto physicalDevicesGraphicsQueueIndex = std::vector<std::optional<u32>>(deviceCount);
	auto physicalDevicesPresentQueueIndex = std::vector<std::optional<u32>>(deviceCount);
	auto physicalDevicesTransferQueueIndex = std::vector<std::optional<u32>>(deviceCount);
	auto physicalDevicesScore = std::vector<i32>(deviceCount);
	// The following three can only be queried after confirming that device extensions are available
	auto physicalDevicesSurfaceCapabilities = std::vector<VkSurfaceCapabilitiesKHR>(deviceCount);
	auto physicalDevicesSurfaceFormats = std::vector<std::vector<VkSurfaceFormatKHR>>(deviceCount);
	auto physicalDevicesSurfacePresentModes = std::vector<std::vector<VkPresentModeKHR>>(deviceCount);
	VK(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));
	for (auto[dev, prop, feat, ext, gfxqueue, presqueue, tranqueue]:
		zip_view{physicalDevices, physicalDevicesProperties, physicalDevicesFeatures,
		         physicalDevicesExtensions, physicalDevicesGraphicsQueueIndex,
		         physicalDevicesPresentQueueIndex, physicalDevicesTransferQueueIndex}) {
		vkGetPhysicalDeviceProperties(dev, &prop);
		vkGetPhysicalDeviceFeatures(dev, &feat);

		// Query for device extensions
		u32 deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &deviceExtensionCount, nullptr);
		ext.resize(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &deviceExtensionCount, ext.data());

		// Query for queue families
		u32 queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
		auto queueFamilies = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

		// Find first queue family that supports graphics
		if (auto const result = ranges::find_if(queueFamilies, [](auto const& family) -> bool {
				return family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
			}); result != queueFamilies.end())
			gfxqueue = ranges::distance(queueFamilies.begin(), result);

		// Find first queue family that supports presentation
		for (u32 familyIndex: ranges::iota_view{0_zu, queueFamilies.size()}) {
			VkBool32 presentSupported;
			VK(vkGetPhysicalDeviceSurfaceSupportKHR(dev, familyIndex, surface,
				&presentSupported));
			if (presentSupported) {
				presqueue = familyIndex;
				break;
			}
		}

		// Check if there exists a dedicated transfer queue
		if (auto const result = ranges::find_if(queueFamilies, [](auto const& family) -> bool {
				return (family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
					!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(family.queueFlags & VK_QUEUE_COMPUTE_BIT);
			}); result != queueFamilies.end())
			tranqueue = ranges::distance(queueFamilies.begin(), result);
	}

	// Query and rate device suitability
	L.info("Available GPUs:");
	for (auto[dev, prop, feat, ext, score, gfxqueue, presqueue, tranqueue, surfcap, surffmt, surfmode]:
		zip_view{physicalDevices, physicalDevicesProperties, physicalDevicesFeatures,
		         physicalDevicesExtensions, physicalDevicesScore, physicalDevicesGraphicsQueueIndex,
		         physicalDevicesPresentQueueIndex, physicalDevicesTransferQueueIndex,
		         physicalDevicesSurfaceCapabilities, physicalDevicesSurfaceFormats,
		         physicalDevicesSurfacePresentModes}) {
		L.info("  {}", prop.deviceName);

		// Version check
		L.info("    Vulkan version: {}", codeToVersion(prop.apiVersion));
		if (prop.apiVersion < vulkanVersion)
			score -= 0x10'00'00;

		// Device feature check
		L.debug("    Multi-draw indirect: {}", feat.multiDrawIndirect? "Supported" : "Unsupported");
		if (!feat.multiDrawIndirect)
			score -= 0x10'00'00;

		// Physical device extension check
		L.debug("    Found {} Vulkan device extensions:", ext.size());
		for (auto const& e: ext)
			L.debug("      {}", e.extensionName);

		for (char const* req: deviceExtensions) {
			auto const result = ranges::find_if(ext, [=](auto const& e) {
				return std::strcmp(req, e.extensionName) == 0;
			});
			if (result == ext.end())
				score -= 0x10'00'00;
		}

		// Swapchain query and check
		if (score >= 0) {
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &surfcap);

			u32 surfaceFormatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &surfaceFormatCount,
				nullptr);
			surffmt.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &surfaceFormatCount,
				surffmt.data());

			u32 surfacePresentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &surfacePresentModeCount,
				nullptr);
			surfmode.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &surfacePresentModeCount,
				surfmode.data());

			if (surffmt.empty() || surfmode.empty())
				score -= 0x10'00'00;
		}

		// Queue family check
		L.debug("    Graphics queue index: {}",
			gfxqueue? std::to_string(gfxqueue.value()) : "N/A"s);
		L.debug("    Presentation queue index: {}",
			presqueue? std::to_string(presqueue.value()) : "N/A"s);
		L.debug("    Transfer queue index: {}",
			tranqueue? std::to_string(tranqueue.value()) : "N/A"s);
		if (!gfxqueue || !presqueue)
			score -= 0x10'00'00;
		if (tranqueue)
			score += 0x1'00'00;

		// Physical device type scoring
		char const* typeStr;
		score += 0x2'00'00 * [&] {
			switch (prop.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: typeStr = "Discrete";
				return 4;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: typeStr = "Integrated";
				return 3;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: typeStr = "Software";
				return 2;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: typeStr = "Virtual";
				return 1;
			default: typeStr = "Unknown";
				return 0;
			}
		}();
		L.info("    Device type: {}", typeStr);

		// Max texture size scoring (tiebreaker)
		L.info("    Max 2D texture size: {}", prop.limits.maxImageDimension2D);
		score += prop.limits.maxImageDimension2D;

		L.info("    Awarded score: {}", score);
	}

	// Choose the best physical device
	auto const bestDeviceIndex = ranges::distance(physicalDevicesScore.begin(),
		ranges::max_element(physicalDevicesScore));
	auto const bestDeviceScore = physicalDevicesScore[bestDeviceIndex];
	if (bestDeviceScore < 0)
		throw std::runtime_error{"Failed to find any suitable GPU"};
	physicalDevice = physicalDevices[bestDeviceIndex];
	deviceProperties = physicalDevicesProperties[bestDeviceIndex];
	graphicsQueueFamilyIndex = physicalDevicesGraphicsQueueIndex[bestDeviceIndex].value();
	presentQueueFamilyIndex = physicalDevicesPresentQueueIndex[bestDeviceIndex].value();
	transferQueueFamilyIndex = physicalDevicesTransferQueueIndex[bestDeviceIndex]?
		physicalDevicesTransferQueueIndex[bestDeviceIndex].value() :
		graphicsQueueFamilyIndex;
	surfaceCapabilities = physicalDevicesSurfaceCapabilities[bestDeviceIndex];
	surfaceFormats = std::move(physicalDevicesSurfaceFormats[bestDeviceIndex]);
	surfacePresentModes = std::move(physicalDevicesSurfacePresentModes[bestDeviceIndex]);
	L.info("Chosen GPU: {} (Score: {})",
		physicalDevicesProperties[bestDeviceIndex].deviceName, bestDeviceScore);

	// Enumerate required queues
	std::vector<VkDeviceQueueCreateInfo> queueCIs;
	queueCIs.reserve(3);
	auto const queuePriority = 1.0f;
	auto queueCI = VkDeviceQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphicsQueueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};
	queueCIs.push_back(queueCI);
	if (uniquePresentQueue()) {
		queueCI.queueFamilyIndex = presentQueueFamilyIndex;
		queueCIs.push_back(queueCI);
	}
	if (uniqueTransferQueue()) {
		queueCI.queueFamilyIndex = transferQueueFamilyIndex;
		queueCIs.push_back(queueCI);
	}

	// Create logical device
	auto const wantedDeviceFeatures = VkPhysicalDeviceFeatures{
		.multiDrawIndirect = VK_TRUE,
	};
	auto const deviceCI = VkDeviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<u32>(queueCIs.size()),
		.pQueueCreateInfos = queueCIs.data(),
#ifdef VK_VALIDATION
		.enabledLayerCount = static_cast<u32>(instanceLayers.size()),
		.ppEnabledLayerNames = instanceLayers.data(),
#endif //VK_VALIDATION
		.enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &wantedDeviceFeatures,
	};
	VK(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
	volkLoadDevice(device);
	vk::setDebugName(device, instance, "Context::instance");
	vk::setDebugName(device, physicalDevice, "Context::physicalDevice");
	vk::setDebugName(device, "Context::device");

	// Retrieve device queues
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
	vkGetDeviceQueue(device, transferQueueFamilyIndex, 0, &transferQueue);
	vk::setDebugName(device, graphicsQueue, "Context::graphicsQueue");
	vk::setDebugName(device, presentQueue, "Context::presentQueue");
	vk::setDebugName(device, transferQueue, "Context::transferQueue");

	// Create the allocator
	auto const allocatorFunctions = VmaVulkanFunctions{
		.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
		.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
		.vkAllocateMemory = vkAllocateMemory,
		.vkFreeMemory = vkFreeMemory,
		.vkMapMemory = vkMapMemory,
		.vkUnmapMemory = vkUnmapMemory,
		.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
		.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
		.vkBindBufferMemory = vkBindBufferMemory,
		.vkBindImageMemory = vkBindImageMemory,
		.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
		.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
		.vkCreateBuffer = vkCreateBuffer,
		.vkDestroyBuffer = vkDestroyBuffer,
		.vkCreateImage = vkCreateImage,
		.vkDestroyImage = vkDestroyImage,
		.vkCmdCopyBuffer = vkCmdCopyBuffer,
		.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
		.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2,
		.vkBindBufferMemory2KHR = vkBindBufferMemory2,
		.vkBindImageMemory2KHR = vkBindImageMemory2,
		.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
	};
	auto const allocatorCI = VmaAllocatorCreateInfo{
		.physicalDevice = physicalDevice,
		.device = device,
		.pVulkanFunctions = &allocatorFunctions,
		.instance = instance,
		.vulkanApiVersion = vulkanVersion,
	};
	VK(vmaCreateAllocator(&allocatorCI, &allocator));

	L.debug("Vulkan device created");
}

void Context::cleanup() {
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
#ifdef VK_VALIDATION
	vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif //VK_VALIDATION
	vkDestroyInstance(instance, nullptr);
}

void Context::refreshSurface() {
	u32 surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount,
		nullptr);
	surfaceFormats.resize(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount,
		surfaceFormats.data());

	u32 surfacePresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount,
		nullptr);
	surfacePresentModes.resize(surfaceFormatCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount,
		surfacePresentModes.data());

	ASSERT(!surfaceFormats.empty() && !surfacePresentModes.empty());
}

#ifdef VK_VALIDATION
VKAPI_ATTR auto VKAPI_CALL Context::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
	VkDebugUtilsMessageTypeFlagsEXT typeCode,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void*) -> VkBool32 {
	ASSERT(data);

	auto const severity = [=] {
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			return Log::Level::Error;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			return Log::Level::Warn;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			return Log::Level::Info;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			return Log::Level::Debug;
		throw std::logic_error{fmt::format("Unknown Vulkan diagnostic message severity: #{}",
			severityCode)};
	}();

	auto type = [=]() -> std::string_view {
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			return "[VulkanPerf]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			return "[VulkanSpec]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			return "[Vulkan]";
		throw std::logic_error{fmt::format("Unknown Vulkan diagnostic message type: #{}",
			typeCode)};
	}();

	L.log(severity, "{} {}", type, data->pMessage);

	return VK_FALSE;
}

#endif //VK_VALIDATION

}
