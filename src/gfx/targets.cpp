#include "gfx/targets.hpp"

#include "sys/vk/framebuffer.hpp"
#include "sys/vk/debug.hpp"
#include "sys/vk/image.hpp"

namespace minote::gfx {

namespace vk = sys::vk;

void Targets::init(Context& ctx, VkExtent2D size, VkFormat color, VkFormat depth) {
	refreshInit(ctx, size, color, depth);
}

void Targets::cleanup(Context& ctx) {
	refreshCleanup(ctx);
}

void Targets::refreshInit(Context& ctx, VkExtent2D size, VkFormat color, VkFormat depth) {
	ssColor = vk::createImage(ctx.device, ctx.allocator, color, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		size);
	vk::setDebugName(ctx.device, ssColor, "Targets::ssColor");
	depthStencil = vk::createImage(ctx.device, ctx.allocator, depth, VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		size);
	vk::setDebugName(ctx.device, depthStencil, "Targets::depthStencil");

	renderPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = ssColor,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
		vk::Attachment{
			.type = vk::Attachment::Type::DepthStencil,
			.image = depthStencil,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.layoutDuring = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	});
	vk::setDebugName(ctx.device, renderPass, "Targets::renderPass");
	framebuffer = vk::createFramebuffer(ctx.device, renderPass, std::array{
		ssColor,
		depthStencil,
	});
	vk::setDebugName(ctx.device, framebuffer, "Targets::framebuffer");
}

void Targets::refreshCleanup(Context& ctx) {
	vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
	vkDestroyRenderPass(ctx.device, renderPass, nullptr);
	vk::destroyImage(ctx.device, ctx.allocator, depthStencil);
	vk::destroyImage(ctx.device, ctx.allocator, ssColor);
}

}
