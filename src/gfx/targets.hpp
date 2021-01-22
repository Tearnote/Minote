#pragma once

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "sys/vk/image.hpp"

namespace minote::gfx {

struct Targets {

	sys::vk::Image msColor;
	sys::vk::Image ssColor;
	sys::vk::Image depthStencil;
	VkRenderPass renderPass;
	VkFramebuffer framebuffer;

	void init(VkDevice device, VmaAllocator allocator, VkExtent2D size,
		VkFormat color, VkFormat depth, VkSampleCountFlagBits samples);
	void cleanup(VkDevice device, VmaAllocator allocator);

	void refreshInit(VkDevice device, VmaAllocator allocator, VkExtent2D size,
		VkFormat color, VkFormat depth, VkSampleCountFlagBits samples);
	void refreshCleanup(VkDevice device, VmaAllocator allocator);

};

}
