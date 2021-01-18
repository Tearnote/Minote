#pragma once

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"

namespace minote::sys::vk {

struct Image {

	VmaAllocation allocation;
	VkImage image;
	VkFormat format;
	VkExtent2D size;

};

auto createImage(VmaAllocator allocator, VkFormat format, VkImageUsageFlags usage,
	VkExtent2D size, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) -> Image;

auto createImageView(VkDevice device, Image& image, VkImageAspectFlags aspect) -> VkImageView;

}
