#pragma once

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "sys/vk/image.hpp"
#include "gfx/context.hpp"

namespace minote::gfx {

struct Targets {

	sys::vk::Image msColor;
	sys::vk::Image ssColor;
	sys::vk::Image depthStencil;
	VkRenderPass renderPass;
	VkFramebuffer framebuffer;

	void init(Context& ctx, VkExtent2D size, VkFormat color, VkFormat depth, VkSampleCountFlagBits samples);
	void cleanup(Context& ctx);

	void refreshInit(Context& ctx, VkExtent2D size, VkFormat color, VkFormat depth, VkSampleCountFlagBits samples);
	void refreshCleanup(Context& ctx);

};

}
