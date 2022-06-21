#pragma once

#include <optional>
#include "volk.h"
#include "VkBootstrap.h"
#include "vuk/Context.hpp"
#include "base/math.hpp"
#include "sys/window.hpp"

namespace minote::sys {

// Initialization and cleanup of the basic Vulkan objects.
struct Vulkan {
	
	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::Device device;
	vuk::SwapchainRef swapchain;
	std::optional<vuk::Context> context;
	
	explicit Vulkan(Window&);
	~Vulkan();
	
	// Create a swapchain object, optionally reusing resources from an existing one.
	auto createSwapchain(uvec2 size, VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	
	// Not copyable, not movable
	Vulkan(Vulkan const&) = delete;
	auto operator=(Vulkan const&) -> Vulkan& = delete;
	
};

}
