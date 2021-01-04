#pragma once

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"

namespace minote::sys::vk {

struct Image {

	VmaAllocation allocation;
	VkImage image;
	VkFormat format;

};

auto createImage(VmaAllocator allocator, VkFormat format, VkImageUsageFlags usage, VkExtent2D size) -> Image;

auto createImageView(VkDevice device, Image& image, VkImageAspectFlags aspect) -> VkImageView;

}
