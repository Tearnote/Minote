#include "sys/vk/commands.hpp"

#include "base/types.hpp"

namespace minote::sys::vk {

using namespace base;

void cmdSetArea(VkCommandBuffer cmdBuf, VkExtent2D size) {
	auto const viewport = VkViewport{
		.width = static_cast<f32>(size.width),
		.height = static_cast<f32>(size.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	auto const scissor = VkRect2D{
		.extent = size,
	};
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
}

void cmdImageBarrier(VkCommandBuffer cmdBuf, Image const& image, VkImageAspectFlags aspect,
	VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess,
	VkImageLayout oldLayout, VkImageLayout newLayout) {
	auto const barrier = VkImageMemoryBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.image = image.image,
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = aspect,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

}
