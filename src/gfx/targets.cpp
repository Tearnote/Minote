#include "gfx/targets.hpp"

#include "sys/vk/framebuffer.hpp"
#include "sys/vk/debug.hpp"
#include "sys/vk/image.hpp"

namespace minote::gfx {

namespace vk = sys::vk;

void Targets::init(VkDevice device, VmaAllocator allocator, VkExtent2D size,
	VkFormat color, VkFormat depth, VkSampleCountFlagBits samples) {
	refreshInit(device, allocator, size, color, depth, samples);
}

void Targets::cleanup(VkDevice device, VmaAllocator allocator) {
	refreshCleanup(device, allocator);
}

void Targets::refreshInit(VkDevice device, VmaAllocator allocator, VkExtent2D size,
	VkFormat color, VkFormat depth, VkSampleCountFlagBits samples) {
	msColor = vk::createImage(device, allocator, color, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		size, samples);
	vk::setDebugName(device, msColor, "Targets::msColor");
	ssColor = vk::createImage(device, allocator, color, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		size);
	vk::setDebugName(device, ssColor, "Targets::ssColor");
	depthStencil = vk::createImage(device, allocator, depth, VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		size, samples);
	vk::setDebugName(device, depthStencil, "Targets::depthStencil");

	renderPass = vk::createRenderPass(device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = msColor,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
		vk::Attachment{
			.type = vk::Attachment::Type::DepthStencil,
			.image = depthStencil,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.layoutDuring = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
		vk::Attachment{
			.type = vk::Attachment::Type::Resolve,
			.image = ssColor,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	vk::setDebugName(device, renderPass, "Targets::renderPass");
	framebuffer = vk::createFramebuffer(device, renderPass, std::array{
		msColor,
		depthStencil,
		ssColor,
	});
	vk::setDebugName(device, framebuffer, "Targets::framebuffer");
}

void Targets::refreshCleanup(VkDevice device, VmaAllocator allocator) {
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vk::destroyImage(device, allocator, depthStencil);
	vk::destroyImage(device, allocator, ssColor);
	vk::destroyImage(device, allocator, msColor);
}

}
