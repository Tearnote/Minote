#pragma once

#include <optional>
#include "volk.h" // Order is important
#include "VkBootstrap.h"
#include "vuk/Context.hpp"
#include "types.hpp"
#include "math.hpp"
#include "util/service.hpp"
#include "sys/system.hpp"

namespace minote::sys {

// Handling of the elementary Vulkan objects
// Currently locked to a single window and swapchain
class Vulkan {

public:
	
	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::PhysicalDevice physicalDevice;
	vkb::Device device;
	vuk::SwapchainRef swapchain;
	std::optional<vuk::Context> context;
	
	// Create a swapchain object, optionally reusing resources from an existing one.
	auto createSwapchain(uint2 size, VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	
	// Return the window instance that Vulkan was initialized on
	auto window() -> Window& { return m_window; }
	
	// Not moveable, not copyable
	Vulkan(Vulkan const&) = delete;
	auto operator=(Vulkan const&) -> Vulkan& = delete;
	
private:
	
	struct Queues {
		VkQueue graphics;
		uint graphicsFamilyIndex;
		VkQueue transfer;
		uint transferFamilyIndex;
		VkQueue compute;
		uint computeFamilyIndex;
	};
	
	// Can only be used as service
	friend struct util::Service<Vulkan>;
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

inline util::Service<Vulkan> s_vulkan;

}
