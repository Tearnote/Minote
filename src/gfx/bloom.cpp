#include "gfx/bloom.hpp"

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

constexpr auto bloomVertSrc = std::to_array<u32>({
#include "spv/bloom.vert.spv"
});

constexpr auto bloomFragSrc = std::to_array<u32>({
#include "spv/bloom.frag.spv"
});

void Bloom::init(Context& ctx, Samplers& samplers, World& world, vk::Image& target, VkFormat format) {
	initImagesFbs(ctx, target, format);

	// Create pipelines
	descriptorSetLayout = vk::createDescriptorSetLayout(ctx.device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.stages = VK_SHADER_STAGE_FRAGMENT_BIT,
			.sampler = samplers.linear,
		},
	});
	vk::setDebugName(ctx.device, descriptorSetLayout, "bloom.descriptorSetLayout");
	shader = vk::createShader(ctx.device, bloomVertSrc, bloomFragSrc);
	vk::setDebugName(ctx.device, shader, "bloom.shader");

	layout = vk::createPipelineLayout(ctx.device, std::array{
		world.getDescriptorSetLayout(),
		descriptorSetLayout,
	});
	vk::setDebugName(ctx.device, layout, "bloom.layout");
	auto builder = vk::PipelineBuilder{
		.shader = shader,
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		.depthStencilStateCI = vk::makePipelineDepthStencilStateCI(false, false, VK_COMPARE_OP_ALWAYS),
		.multisampleStateCI = vk::makePipelineMultisampleStateCI(),
		.layout = layout,
	};
	down = builder.build(ctx.device, downPass);
	vk::setDebugName(ctx.device, down, "bloom.down");
	builder.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Add);
	up = builder.build(ctx.device, upPass);
	vk::setDebugName(ctx.device, up, "bloom.up");

	initDescriptorSet(ctx, target);
}

void Bloom::cleanup(Context& ctx) {
	cleanupDescriptorSet(ctx);

	vkDestroyPipeline(ctx.device, up, nullptr);
	vkDestroyPipeline(ctx.device, down, nullptr);
	vkDestroyPipelineLayout(ctx.device, layout, nullptr);
	vk::destroyShader(ctx.device, shader);
	vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);

	cleanupImagesFbs(ctx);
}

void Bloom::refreshInit(Context& ctx, vk::Image& target, VkFormat format) {
	initImagesFbs(ctx, target, format);
	initDescriptorSet(ctx, target);
}

void Bloom::refreshCleanup(Context& ctx) {
	cleanupDescriptorSet(ctx);
	cleanupImagesFbs(ctx);
}

void Bloom::initImagesFbs(Context& ctx, vk::Image& target, VkFormat format) {
	auto extent = target.size;
	for (auto& image: images) {
		image = vk::createImage(ctx.device, ctx.allocator, format, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
		vk::setDebugName(ctx.device, image, fmt::format("bloom.images[{}]", &image - &images[0]));
		extent.width = std::max(1u, extent.width >> 1);
		extent.height = std::max(1u, extent.height >> 1);
	}

	downPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = images[0],
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	vk::setDebugName(ctx.device, downPass, "bloom.downPass");
	upPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = images[0],
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutBefore = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	vk::setDebugName(ctx.device, upPass, "bloom.upPass");

	targetFb = vk::createFramebuffer(ctx.device, downPass, std::array{target});
	vk::setDebugName(ctx.device, targetFb, "bloom.targetFb");
	for (auto[fb, image]: zip_view{imageFbs, images}) {
		fb = vk::createFramebuffer(ctx.device, downPass, std::array{image});
		vk::setDebugName(ctx.device, fb, fmt::format("bloom.imageFbs[{}]", &fb - &imageFbs[0]));
	}
}

void Bloom::cleanupImagesFbs(Context& ctx) {
	vkDestroyFramebuffer(ctx.device, targetFb, nullptr);
	for (auto fb: imageFbs)
		vkDestroyFramebuffer(ctx.device, fb, nullptr);
	vkDestroyRenderPass(ctx.device, upPass, nullptr);
	vkDestroyRenderPass(ctx.device, downPass, nullptr);

	for (auto& image: images)
		vk::destroyImage(ctx.device, ctx.allocator, image);
}

void Bloom::initDescriptorSet(Context& ctx, vk::Image& target) {
	sourceDS = vk::allocateDescriptorSet(ctx.device, ctx.descriptorPool, descriptorSetLayout);
	vk::setDebugName(ctx.device, sourceDS, "bloom.sourceDS");
	for (auto& ds: imageDS) {
		ds = vk::allocateDescriptorSet(ctx.device, ctx.descriptorPool, descriptorSetLayout);
		vk::setDebugName(ctx.device, ds, fmt::format("bloom.imageDS[{}]", &ds - &imageDS[0]));
	}

	vk::updateDescriptorSets(ctx.device, std::array{
		vk::makeDescriptorSetImageWrite(sourceDS, 0, target,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	});
	for (auto[ds, image]: zip_view{imageDS, images}) {
		vk::updateDescriptorSets(ctx.device, std::array{
			vk::makeDescriptorSetImageWrite(ds, 0, image,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		});
	}
}

void Bloom::cleanupDescriptorSet(Context& ctx) {
	for (auto& ds: imageDS)
		vkFreeDescriptorSets(ctx.device, ctx.descriptorPool, 1, &ds);
	vkFreeDescriptorSets(ctx.device, ctx.descriptorPool, 1, &sourceDS);
}

}
