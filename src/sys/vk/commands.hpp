#pragma once

#include <span>
#include "volk/volk.h"
#include "sys/vk/image.hpp"

namespace minote::sys::vk {

void cmdSetArea(VkCommandBuffer cmdBuf, VkExtent2D size);

void cmdImageBarrier(VkCommandBuffer cmdBuf, Image const& image, VkImageAspectFlags aspect,
	VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess,
	VkImageLayout oldLayout, VkImageLayout newLayout);

void cmdBeginRenderPass(VkCommandBuffer cmdBuf, VkRenderPass renderPass,
	VkFramebuffer fb, VkExtent2D extent, std::span<VkClearValue const> clearValues = {});

}
