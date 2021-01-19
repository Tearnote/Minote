#pragma once

#include <span>
#include "volk/volk.h"
#include "sys/vk/image.hpp"

namespace minote::sys::vk {

struct Attachment {

	enum struct Type {
		Input,
		Color,
		DepthStencil,
		Resolve,
	} type;

	Image& image;
	VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	VkImageLayout layoutBefore = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout layoutDuring = VK_IMAGE_LAYOUT_UNDEFINED; // Set to layoutBefore if undefined
	VkImageLayout layoutAfter = VK_IMAGE_LAYOUT_UNDEFINED; // Set to layoutDuring if undefined

};

auto createRenderPass(VkDevice device, std::span<Attachment const> attachments) -> VkRenderPass;

auto createFramebuffer(VkDevice device, VkRenderPass renderPass, std::span<Image const> attachments) -> VkFramebuffer;

}
