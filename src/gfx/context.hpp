#pragma once

#include <string_view>
#include <vector>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/version.hpp"
#include "base/types.hpp"
#include "sys/vk/base.hpp"
#include "sys/window.hpp"

namespace minote::gfx {

using namespace base;

struct Context {

	std::string name;
	sys::Window* window;

	VkInstance instance;
	std::vector<char const*> instanceExtensions;
#ifdef VK_VALIDATION
	std::vector<char const*> instanceLayers;
	VkDebugUtilsMessengerEXT debugMessenger;
#endif //VK_VALIDATION
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	std::vector<char const*> deviceExtensions;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> surfacePresentModes;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	u32 graphicsQueueFamilyIndex;
	u32 presentQueueFamilyIndex;
	u32 transferQueueFamilyIndex;

	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	VmaAllocator allocator;

	void init(sys::Window& window, u32 vulkanVersion, std::string_view appName, Version appVersion);
	void cleanup();

	void refreshSurface();

	[[nodiscard]]
	auto uniquePresentQueue() const {
		return (presentQueueFamilyIndex != graphicsQueueFamilyIndex);
	}

	[[nodiscard]]
	auto uniqueTransferQueue() const {
		return (transferQueueFamilyIndex != graphicsQueueFamilyIndex);
	}

private:

#ifdef VK_VALIDATION
	static VKAPI_ATTR auto VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
		VkDebugUtilsMessageTypeFlagsEXT typeCode,
		VkDebugUtilsMessengerCallbackDataEXT const* data,
		void*) -> VkBool32;
#endif //VK_VALIDATION

};

}
