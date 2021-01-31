#include "gfx/present.hpp"

#include <fmt/core.h>
#include "base/zip_view.hpp"
#include "sys/vk/framebuffer.hpp"
#include "sys/vk/descriptor.hpp"
#include "sys/vk/pipeline.hpp"
#include "sys/vk/shader.hpp"
#include "sys/vk/debug.hpp"

namespace minote::gfx {

using namespace base;
namespace vk = sys::vk;

constexpr auto presentVertSrc = std::to_array<u32>({
#include "spv/present.vert.spv"
});

constexpr auto presentFragSrc = std::to_array<u32>({
#include "spv/present.frag.spv"
});

void Present::init(Context& ctx, World& world, sys::vk::Image& source, Swapchain& swapchain) {
	initFbs(ctx, source, swapchain);

	// Create the pipeline
	descriptorSetLayout = vk::createDescriptorSetLayout(ctx.device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.stages = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	});
	vk::setDebugName(ctx.device, descriptorSetLayout, "Present::descriptorSetLayout");
	shader = vk::createShader(ctx.device, presentVertSrc, presentFragSrc);
	vk::setDebugName(ctx.device, shader, "Present::shader");

	layout = vk::createPipelineLayout(ctx.device, std::array{
		world.getDescriptorSetLayout(),
		descriptorSetLayout,
	});
	vk::setDebugName(ctx.device, layout, "Present::layout");
	pipeline = vk::PipelineBuilder{
		.shader = shader,
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		.depthStencilStateCI = vk::makePipelineDepthStencilStateCI(false, false, VK_COMPARE_OP_ALWAYS),
		.layout = layout,
	}.build(ctx.device, renderPass);
	vk::setDebugName(ctx.device, pipeline, "Present::pipeline");

	initDescriptorSet(ctx, source);
}

void Present::cleanup(Context& ctx) {
	cleanupDescriptorSet(ctx);

	vkDestroyPipeline(ctx.device, pipeline, nullptr);
	vkDestroyPipelineLayout(ctx.device, layout, nullptr);
	vk::destroyShader(ctx.device, shader);
	vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);

	cleanupFbs(ctx);
}

void Present::refreshInit(Context& ctx, sys::vk::Image& source, Swapchain& swapchain) {
	initFbs(ctx, source, swapchain);
	initDescriptorSet(ctx, source);
}

void Present::refreshCleanup(Context& ctx) {
	cleanupDescriptorSet(ctx);
	cleanupFbs(ctx);
}

void Present::initFbs(Context& ctx, sys::vk::Image& source, Swapchain& swapchain) {
	// Create the present render pass
	renderPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{ // Source
			.type = vk::Attachment::Type::Input,
			.image = source,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.layoutBefore = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		},
		vk::Attachment{ // Present
			.type = vk::Attachment::Type::Color,
			.image = swapchain.color[0],
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.layoutAfter = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
	});
	vk::setDebugName(ctx.device, renderPass, "Present::renderPass");

	// Create the present framebuffers
	framebuffer.resize(swapchain.color.size());
	for (auto[fb, image]: zip_view{framebuffer, swapchain.color}) {
		fb = vk::createFramebuffer(ctx.device, renderPass, std::array{source, image});
		vk::setDebugName(ctx.device, fb, fmt::format("Present::framebuffer[{}]", &fb - &framebuffer[0]));
	}
}

void Present::cleanupFbs(Context& ctx) {
	for (auto& fb: framebuffer)
		vkDestroyFramebuffer(ctx.device, fb, nullptr);
	vkDestroyRenderPass(ctx.device, renderPass, nullptr);
}

void Present::initDescriptorSet(Context& ctx, sys::vk::Image& source) {
	descriptorSet = vk::allocateDescriptorSet(ctx.device, ctx.descriptorPool, descriptorSetLayout);
	vk::setDebugName(ctx.device, descriptorSet, "present.descriptorSet");
	vk::updateDescriptorSets(ctx.device, std::array{
		vk::makeDescriptorSetImageWrite(descriptorSet, 0, source,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	});
}

void Present::cleanupDescriptorSet(Context& ctx) {
	vkFreeDescriptorSets(ctx.device, ctx.descriptorPool, 1, &descriptorSet);
}

}
