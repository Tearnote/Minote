#pragma once

#include "volk.h"
#include "VkBootstrap.h"
#include "vuk/Context.hpp"
#include "util/optional.hpp"
#include "util/service.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "sys/system.hpp"

namespace minote {

// Handling of the elementary Vulkan objects
// Currently locked to a single window and swapchain
struct Vulkan {
	
	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::PhysicalDevice physicalDevice;
	vkb::Device device;
	vuk::SwapchainRef swapchain;
	optional<vuk::Context> context;
	
	// Create a swapchain object, optionally reusing resources from an existing one.
	auto createSwapchain(uvec2 size, VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	
	// Return the window instance that Vulkan was initialized on
	auto window() -> Window& { return m_window; }
	
	Vulkan(Vulkan const&) = delete;
	auto operator=(Vulkan const&) -> Vulkan& = delete;
	
private:
	
	struct Queues {
		VkQueue graphics;
		u32 graphicsFamilyIndex;
		VkQueue transfer;
		u32 transferFamilyIndex;
		VkQueue compute;
		u32 computeFamilyIndex;
	};
	
	friend struct Service<Vulkan>;
	explicit Vulkan(Window&);
	~Vulkan();
	
	Window& m_window;
	
	static auto createInstance() -> vkb::Instance;
	static auto createSurface(vkb::Instance&, Window&) -> VkSurfaceKHR;
	static auto selectPhysicalDevice(vkb::Instance&, VkSurfaceKHR) -> vkb::PhysicalDevice;
	static auto createDevice(vkb::PhysicalDevice&) -> vkb::Device;
	static auto retrieveQueues(vkb::Device&) -> Queues;
	static auto createContext(vkb::Instance&, vkb::Device&, vkb::PhysicalDevice&, Queues const&) -> vuk::Context;
	
};

inline Service<Vulkan> s_vulkan;

}
