#pragma once

#include <optional>
#include "VkBootstrap.h"
#include "volk.h"
#include "vuk/Context.hpp"
#include "sys/window.hpp"

namespace minote::sys {

struct Vulkan {
	
	explicit Vulkan(Window&);
	~Vulkan();
	
	// Create a swapchain object, optionally reusing resources from an existing one.
	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	
	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::Device device;
	vuk::SwapChainRef swapchain;
	std::optional<vuk::Context> context;
	
};


}
