#pragma once

#include "volk/volk.h"
#include "sys/vk/image.hpp"

namespace minote::sys::vk {

void cmdSetArea(VkCommandBuffer cmdBuf, VkExtent2D size);

void cmdImageBarrier(VkCommandBuffer cmdBuf, Image const& image, VkImageAspectFlags aspect,
	VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess,
	VkImageLayout oldLayout, VkImageLayout newLayout);

}
