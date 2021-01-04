#pragma once

#include <string_view>
#include <string>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/hashmap.hpp"
#include "base/svector.hpp"
#include "base/version.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "base/id.hpp"
#include "sys/vk/image.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "engine/technique.hpp"
#include "engine/base.hpp"
#include "engine/mesh.hpp"

#ifndef NDEBUG
#define VK_VALIDATION
#endif //NDEBUG

namespace minote::engine {

using namespace base::literals;

struct Engine {

	struct Frame {

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
		VkSemaphore renderSemaphore;
		VkSemaphore presentSemaphore;
		VkFence renderFence;

	};

	struct SwapchainFb {

		sys::vk::Image color;
		VkImageView colorView;
		VkFramebuffer framebuffer;

	};

	struct Swapchain {

		VkSwapchainKHR swapchain = nullptr;
		sys::vk::Image depthStencil;
		VkImageView depthStencilView;
		VkFormat format;
		VkExtent2D extent;
		std::vector<SwapchainFb> fbs;
		VkRenderPass renderPass;
		base::i64 expiry;

	};

	std::string name;
	sys::Window& window;
	base::i64 frameCounter;

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
	base::u32 graphicsQueueFamilyIndex;
	base::u32 presentQueueFamilyIndex;
	base::u32 transferQueueFamilyIndex;

	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	VmaAllocator allocator;

	Swapchain swapchain;
	base::ring<Swapchain, 16> oldSwapchains;

	PerFrame<Frame> frames;
	VkDescriptorPool descriptorPool;
	VkCommandPool transferCommandPool;
	VkFence transfersFinished;

	TechniqueSet techniques;
	MeshBuffer meshes;

	Engine(sys::Glfw&, sys::Window&, std::string_view name, base::Version appVersion);
	~Engine();

	[[nodiscard]]
	auto uniquePresentQueue() const {
		return (presentQueueFamilyIndex != graphicsQueueFamilyIndex);
	}

	[[nodiscard]]
	auto uniqueTransferQueue() const {
		return (transferQueueFamilyIndex != graphicsQueueFamilyIndex);
	}

	void queueScene();
	void render();

private:

	void initInstance(base::Version appVersion);
	void initPhysicalDevice();
	void initDevice();
	void initAllocator();
	void initSwapchain(VkSwapchainKHR old = nullptr);
	void initFrames();
	void initContent();

	void destroySwapchain(Swapchain&);
	void recreateSwapchain();

#ifdef VK_VALIDATION
	static VKAPI_ATTR auto VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
		VkDebugUtilsMessageTypeFlagsEXT typeCode,
		VkDebugUtilsMessengerCallbackDataEXT const* data,
		void*) -> VkBool32;
#endif //VK_VALIDATION

};

}
