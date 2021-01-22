#include "gfx/technique.hpp"

#include <utility>
#include <fmt/core.h>
#include "base/zip_view.hpp"
#include "sys/vk/descriptor.hpp"
#include "sys/vk/pipeline.hpp"
#include "sys/vk/debug.hpp"

namespace minote::gfx {

using namespace base;
namespace vk = sys::vk;

constexpr auto objectVertSrc = std::to_array<u32>({
#include "spv/object.vert.spv"
});

constexpr auto objectFragSrc = std::to_array<u32>({
#include "spv/object.frag.spv"
});

void TechniqueSet::create(Context& ctx, VkDescriptorSetLayout worldLayout) {
	m_shader = vk::createShader(ctx.device, objectVertSrc, objectFragSrc);
	vk::setDebugName(ctx.device, m_shader, "TechniqueSet::m_shader");

	// Create the shader-specific descriptor set layout
	m_drawDescriptorSetLayout = vk::createDescriptorSetLayout(ctx.device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	});
	vk::setDebugName(ctx.device, m_drawDescriptorSetLayout, "TechniqueSet::m_drawDescriptorSetLayout");

	// Create the pipeline layout
	m_pipelineLayout = vk::createPipelineLayout(ctx.device, std::array{
		worldLayout,
		m_drawDescriptorSetLayout,
	});
	vk::setDebugName(ctx.device, m_pipelineLayout, "TechniqueSet::m_pipelineLayout");
}

void TechniqueSet::destroy(Context& ctx) {
	for (auto[id, tech]: m_techniques) {
		for (auto& indirect: tech.indirect)
			indirect.destroy(ctx);
		vkDestroyPipeline(ctx.device, tech.pipeline, nullptr);
	}
	vkDestroyPipelineLayout(ctx.device, m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(ctx.device, m_drawDescriptorSetLayout, nullptr);
	vk::destroyShader(ctx.device, m_shader);
}

void TechniqueSet::addTechnique(Context& ctx, base::ID id, VkRenderPass renderPass,
	PerFrame<VkDescriptorSet> worldDescriptorSets,
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI,
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI,
	VkPipelineMultisampleStateCreateInfo multisampleStateCI) {
	Technique result;

	// Create the technique's pipeline
	result.pipeline = vk::PipelineBuilder{
		.shader = m_shader,
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = rasterizationStateCI,
		.colorBlendAttachmentState = colorBlendAttachmentState,
		.depthStencilStateCI = depthStencilStateCI,
		.multisampleStateCI = multisampleStateCI,
		.layout = m_pipelineLayout,
	}.build(ctx.device, renderPass);

	// Create the technique's draw (slot 1) descriptor set
	for (auto[drawDS, world, indirect]: zip_view{result.drawDescriptorSet, worldDescriptorSets, result.indirect}) {
		drawDS = vk::allocateDescriptorSet(ctx.device, ctx.descriptorPool, m_drawDescriptorSetLayout);

		// Create the indirect buffer
		indirect.create(ctx, MaxDrawCommands, MaxInstances);

		// Fill in the draw descriptor set
		vk::updateDescriptorSets(ctx.device, std::array{
			vk::makeDescriptorSetBufferWrite(drawDS, 0, indirect.commandBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			vk::makeDescriptorSetBufferWrite(drawDS, 1, indirect.instanceBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		});
	}

	m_techniques.emplace(id, std::move(result));
}

void TechniqueSet::setTechniqueDebugName(Context& ctx, ID id, std::string_view name) {
	auto const& technique = m_techniques.at(id);
	vk::setDebugName(ctx.device, technique.pipeline, fmt::format("TechniqueSet[{}].pipeline", name));
	for (auto[drawDS, indirect]: zip_view{technique.drawDescriptorSet, technique.indirect}) {
		vk::setDebugName(ctx.device, drawDS, fmt::format("TechniqueSet[{}].drawDescriptorSet[{}]",
			name, &drawDS - &technique.drawDescriptorSet[0]));
		indirect.setDebugName(ctx, fmt::format("TechniqueSet[{}].indirect[{}]",
			name, &indirect - &technique.indirect[0]));
	}
}

}
