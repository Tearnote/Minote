#pragma once

#include <string_view>
#include <string>
#include <span>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/hashmap.hpp"
#include "base/svector.hpp"
#include "base/version.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "base/id.hpp"
#include "sys/vk/buffer.hpp"
#include "sys/vk/image.hpp"
#include "sys/vk/base.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "gfx/technique.hpp"
#include "gfx/targets.hpp"
#include "gfx/world.hpp"
#include "gfx/base.hpp"
#include "gfx/mesh.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct Engine {

	Engine(sys::Glfw&, sys::Window&, std::string_view name, Version appVersion);
	~Engine();

	void setBackground(glm::vec3 color);
	void setLightSource(glm::vec3 position, glm::vec3 color);
	void setCamera(glm::vec3 eye, glm::vec3 center, glm::vec3 up = {0.0f, 1.0f, 0.0f});

	void enqueueDraw(ID mesh, ID technique, std::span<Instance const> instances,
		Material material, MaterialData const& materialData = {});

	void render();

private:

	struct Swapchain {

		VkSwapchainKHR swapchain = nullptr;
		VkExtent2D extent = {};
		std::vector<sys::vk::Image> color;

	};

	struct Present {

		VkRenderPass renderPass;
		std::vector<VkFramebuffer> framebuffer;
		VkDescriptorSetLayout descriptorSetLayout;
		sys::vk::Shader shader;
		VkPipelineLayout layout;
		VkPipeline pipeline;
		VkDescriptorSet descriptorSet;

	};

	struct Bloom {

		static constexpr auto Depth = 6_zu;

		std::array<sys::vk::Image, Depth> images;
		VkRenderPass downPass;
		VkRenderPass upPass;
		std::array<VkFramebuffer, Depth> imageFbs;
		VkFramebuffer targetFb;
		VkDescriptorSetLayout descriptorSetLayout;
		sys::vk::Shader shader;
		VkPipelineLayout layout;
		VkPipeline down;
		VkPipeline up;
		VkDescriptorSet sourceDS;
		std::array<VkDescriptorSet, Depth> imageDS;

	};

	struct Frame {

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
		VkSemaphore renderSemaphore;
		VkSemaphore presentSemaphore;
		VkFence renderFence;

	};

	struct Camera {


		glm::vec3 eye;
		glm::vec3 center;
		glm::vec3 up;
	};

	struct DelayedOp {

		u64 deadline;
		std::function<void()> func;

	};

	std::string name;
	sys::Window& window;
	u64 frameCounter;
	svector<DelayedOp, 64> delayedOps;

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

	PerFrame<Frame> frames;
	VkDescriptorPool descriptorPool;
	VkCommandPool transferCommandPool;
	VkFence transfersFinished;

	Swapchain swapchain;
	Present present;
	Targets targets;
	Bloom bloom;

	VkSampler linear;

	TechniqueSet techniques;
	MeshBuffer meshes;
	Camera camera;
	World world;

	[[nodiscard]]
	auto uniquePresentQueue() const {
		return (presentQueueFamilyIndex != graphicsQueueFamilyIndex);
	}

	[[nodiscard]]
	auto uniqueTransferQueue() const {
		return (transferQueueFamilyIndex != graphicsQueueFamilyIndex);
	}

	void initInstance(Version appVersion);
	void cleanupInstance();

	void initPhysicalDevice();

	void initDevice();
	void cleanupDevice();

	void initCommands();
	void cleanupCommands();

	void initSamplers();
	void cleanupSamplers();

	void initSwapchain();
	void cleanupSwapchain();

	void initImages();
	void cleanupImages();

	void initFramebuffers();
	void cleanupFramebuffers();

	void initBuffers();
	void cleanupBuffers();

	void initPipelines();
	void cleanupPipelines();

	void createSwapchain(VkSwapchainKHR old = nullptr);
	void destroySwapchain(Swapchain&);
	void recreateSwapchain();

	void createPresentFbs();
	void destroyPresentFbs(Present&);
	void createPresentPipeline();
	void destroyPresentPipeline();
	void createPresentPipelineDS();
	void destroyPresentPipelineDS(Present&);

	void createMeshBuffer(VkCommandBuffer, std::vector<sys::vk::Buffer>& staging);
	void destroyMeshBuffer();

	void createBloomImages();
	void destroyBloomImages(Bloom&);
	void createBloomFbs();
	void destroyBloomFbs(Bloom&);
	void createBloomPipelines();
	void destroyBloomPipelines();
	void createBloomPipelineDS();
	void destroyBloomPipelineDS(Bloom&);

#ifdef VK_VALIDATION
	static VKAPI_ATTR auto VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
		VkDebugUtilsMessageTypeFlagsEXT typeCode,
		VkDebugUtilsMessengerCallbackDataEXT const* data,
		void*) -> VkBool32;
#endif //VK_VALIDATION

};

}
