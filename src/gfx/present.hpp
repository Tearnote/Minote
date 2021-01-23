#pragma once

#include <vector>
#include "volk/volk.h"
#include "sys/vk/shader.hpp"
#include "sys/vk/image.hpp"
#include "gfx/swapchain.hpp"
#include "gfx/context.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

struct Present {

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffer;
	VkDescriptorSetLayout descriptorSetLayout;
	sys::vk::Shader shader;
	VkPipelineLayout layout;
	VkPipeline pipeline;
	VkDescriptorSet descriptorSet;

	void init(Context& ctx, World& world, sys::vk::Image& source, Swapchain& swapchain);
	void cleanup(Context& ctx);

	void refreshInit(Context& ctx, sys::vk::Image& source, Swapchain& swapchain);
	void refreshCleanup(Context& ctx);

private:

	void initFbs(Context& ctx, sys::vk::Image& source, Swapchain& swapchain);
	void cleanupFbs(Context& ctx);

	void initDescriptorSet(Context& ctx, sys::vk::Image& source);
	void cleanupDescriptorSet(Context& ctx);

};

}
