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
#include "gfx/swapchain.hpp"
#include "gfx/technique.hpp"
#include "gfx/samplers.hpp"
#include "gfx/context.hpp"
#include "gfx/targets.hpp"
#include "gfx/bloom.hpp"
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

	struct Present {

		VkRenderPass renderPass;
		std::vector<VkFramebuffer> framebuffer;
		VkDescriptorSetLayout descriptorSetLayout;
		sys::vk::Shader shader;
		VkPipelineLayout layout;
		VkPipeline pipeline;
		VkDescriptorSet descriptorSet;

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

	u64 frameCounter = 0;
	svector<DelayedOp, 64> delayedOps;

	Context ctx;
	Swapchain swapchain;
	PerFrame<Frame> frames;
	VkCommandPool transferCommandPool;
	VkFence transfersFinished;

	Samplers samplers;
	MeshBuffer meshes;
	World world;
	Camera camera;
	Targets targets;

	TechniqueSet techniques;
	Bloom bloom;
	Present present;

	void initCommands();
	void cleanupCommands();

	void initImages();
	void cleanupImages();

	void initFramebuffers();
	void cleanupFramebuffers();

	void initPipelines();
	void cleanupPipelines();
	void refresh();

	void createPresentFbs();
	void destroyPresentFbs(Present&);
	void createPresentPipeline();
	void destroyPresentPipeline();
	void createPresentPipelineDS();
	void destroyPresentPipelineDS(Present&);

	void createMeshBuffer(VkCommandBuffer, std::vector<sys::vk::Buffer>& staging);
	void destroyMeshBuffer();

};

}
