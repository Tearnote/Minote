#include "sys/vk/image.hpp"

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

auto createImage(VmaAllocator allocator, VkFormat format, VkImageUsageFlags usage, VkExtent2D size) -> Image {
	auto const extent = VkExtent3D{size.width, size.height, 1};
	auto const imageCreateCI = VkImageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
	};
	auto const allocationCI = VmaAllocationCreateInfo{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};

	Image result;
	VK(vmaCreateImage(allocator, &imageCreateCI, &allocationCI, &result.image, &result.allocation, nullptr));
	result.format = format;
	return result;
}

auto createImageView(VkDevice device, Image& image, VkImageAspectFlags aspect) -> VkImageView {
	auto imageViewCI = VkImageViewCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = image.format,
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkImageView result;
	VK(vkCreateImageView(device, &imageViewCI, nullptr, &result));
	return result;
}

}
