#pragma once

#include <array>
#include "volk/volk.h"
#include "base/util.hpp"
#include "sys/vk/shader.hpp"
#include "sys/vk/image.hpp"
#include "gfx/samplers.hpp"
#include "gfx/context.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

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

	void init(Context& ctx, Samplers& samplers, World& world, sys::vk::Image& target, VkFormat format);
	void cleanup(Context& ctx);

	void refreshInit(Context& ctx, sys::vk::Image& target, VkFormat format);
	void refreshCleanup(Context& ctx);

private:

	void initImagesFbs(Context& ctx, sys::vk::Image& target, VkFormat format);
	void cleanupImagesFbs(Context& ctx);

	void initDescriptorSet(Context& ctx, sys::vk::Image& target);
	void cleanupDescriptorSet(Context& ctx);

};

}
