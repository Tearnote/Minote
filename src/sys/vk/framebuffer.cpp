#include "sys/vk/framebuffer.hpp"

#include <stdexcept>
#include <vector>
#include <ranges>
#include "base/assert.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;
using namespace base::literals;
namespace ranges = std::ranges;

auto createRenderPass(VkDevice device, std::span<Attachment const> attachments) -> VkRenderPass {
	// Create list of attachment descriptions
	std::vector<VkAttachmentDescription> rpAttachments;
	rpAttachments.reserve(attachments.size());
	for(auto const& at: attachments) {
		rpAttachments.emplace_back(VkAttachmentDescription{
			.format = at.image.format,
			.samples = at.image.samples,
			.loadOp = at.loadOp,
			.storeOp = at.storeOp,
			.stencilLoadOp = at.stencilLoadOp,
			.stencilStoreOp = at.stencilStoreOp,
			.initialLayout = at.layoutBefore,
			.finalLayout = at.layoutAfter != VK_IMAGE_LAYOUT_UNDEFINED? at.layoutAfter :
				at.layoutDuring != VK_IMAGE_LAYOUT_UNDEFINED? at.layoutDuring : at.layoutBefore,
		});
	}

	// Create lists of attachment references
	std::vector<VkAttachmentReference> inputAttachments;
	std::vector<VkAttachmentReference> colorAttachments;
	std::vector<VkAttachmentReference> dsAttachments;
	std::vector<VkAttachmentReference> resolveAttachments;
	inputAttachments.reserve(attachments.size());
	colorAttachments.reserve(attachments.size());
	dsAttachments.reserve(1);
	resolveAttachments.reserve(attachments.size());
	for(auto i: nrange(0_zu, attachments.size())) {
		auto& vec = [&, i]() -> std::vector<VkAttachmentReference>& {
			switch (attachments[i].type) {
			case Attachment::Type::Input: return inputAttachments;
			case Attachment::Type::Color: return colorAttachments;
			case Attachment::Type::DepthStencil: return dsAttachments;
			case Attachment::Type::Resolve: return resolveAttachments;
			default: throw std::logic_error{"Unknown render pass attachment type"};
			}
		}();
		vec.emplace_back(VkAttachmentReference{
			.attachment = static_cast<u32>(i),
			.layout = attachments[i].layoutDuring != VK_IMAGE_LAYOUT_UNDEFINED?
				attachments[i].layoutDuring : attachments[i].layoutBefore,
		});
	}

	// Fill in initializer structs
	ASSERT(resolveAttachments.empty() || colorAttachments.size() == resolveAttachments.size());
	ASSERT(dsAttachments.size() <= 1_zu);
	auto const subpass = VkSubpassDescription{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = static_cast<u32>(inputAttachments.size()),
		.pInputAttachments = !inputAttachments.empty()? inputAttachments.data() : nullptr,
		.colorAttachmentCount = static_cast<u32>(colorAttachments.size()),
		.pColorAttachments = !colorAttachments.empty()? colorAttachments.data() : nullptr,
		.pResolveAttachments = !resolveAttachments.empty()? resolveAttachments.data() : nullptr,
		.pDepthStencilAttachment = !dsAttachments.empty()? dsAttachments.data() : nullptr,
	};
	auto const renderPassCI = VkRenderPassCreateInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<u32>(rpAttachments.size()),
		.pAttachments = rpAttachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	VkRenderPass result;
	VK(vkCreateRenderPass(device, &renderPassCI, nullptr, &result));
	return result;
}

auto createFramebuffer(VkDevice device, VkRenderPass renderPass, std::span<Image const> attachments) -> VkFramebuffer {
	auto const fbSize = attachments[0].size;
	std::vector<VkImageView> fbAttachments;
	fbAttachments.reserve(attachments.size());
	ranges::transform(attachments, std::back_inserter(fbAttachments), [&](auto& image) {
		ASSERT(fbSize.width == image.size.width && fbSize.height == image.size.height);
		return image.view;
	});

	auto const framebufferCI = VkFramebufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = static_cast<u32>(fbAttachments.size()),
		.pAttachments = fbAttachments.data(),
		.width = fbSize.width,
		.height = fbSize.height,
		.layers = 1,
	};
	VkFramebuffer result;
	VK(vkCreateFramebuffer(device, &framebufferCI, nullptr, &result));
	return result;
}

}
