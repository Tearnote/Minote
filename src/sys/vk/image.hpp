#pragma once

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"

namespace minote::sys::vk {

struct Image {

	VmaAllocation allocation;
	VkImage image;
	VkImageView view;
	VkFormat format;
	VkImageAspectFlags aspect;
	VkSampleCountFlagBits samples;
	VkExtent2D size;

};

auto createImage(VkDevice device, VmaAllocator allocator, VkFormat format,
	VkImageAspectFlags aspect, VkImageUsageFlags usage, VkExtent2D size,
	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) -> Image;

void destroyImage(VkDevice device, VmaAllocator allocator, Image& image);

auto createImageView(VkDevice device, Image& image) -> VkImageView;

}
