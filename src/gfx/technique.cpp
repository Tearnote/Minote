#include "gfx/technique.hpp"

#include <utility>
#include "base/zip_view.hpp"
#include "sys/vk/descriptor.hpp"
#include "sys/vk/pipeline.hpp"

namespace minote::gfx {

using namespace base;
namespace vk = sys::vk;

constexpr auto objectVertSrc = std::to_array<u32>({
#include "spv/object.vert.spv"
});

constexpr auto objectFragSrc = std::to_array<u32>({
#include "spv/object.frag.spv"
});

void TechniqueSet::create(VkDevice device, VkDescriptorSetLayout worldLayout) {
	m_shader = vk::createShader(device, objectVertSrc, objectFragSrc);

	// Create the shader-specific descriptor set layout
	m_drawDescriptorSetLayout = vk::createDescriptorSetLayout(device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	});

	// Create the pipeline layout
	m_pipelineLayout = vk::createPipelineLayout(device, std::array{
		worldLayout,
		m_drawDescriptorSetLayout,
	});
}

void TechniqueSet::destroy(VkDevice device, VmaAllocator allocator) {
	for (auto[id, tech]: m_techniques) {
		for (auto& indirect: tech.indirect)
			indirect.destroy(allocator);
		vkDestroyPipeline(device, tech.pipeline, nullptr);
	}
	vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, m_drawDescriptorSetLayout, nullptr);
	vk::destroyShader(device, m_shader);
}

void TechniqueSet::addTechnique(base::ID id, VkDevice device, VmaAllocator allocator, VkRenderPass renderPass,
	VkDescriptorPool descriptorPool, PerFrame<VkDescriptorSet> worldDescriptorSets,
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
	}.build(device, renderPass);

	// Create the technique's draw (slot 1) descriptor set
	for (auto[drawDS, world, indirect]: zip_view{result.drawDescriptorSet, worldDescriptorSets, result.indirect}) {
		drawDS = vk::allocateDescriptorSet(device, descriptorPool, m_drawDescriptorSetLayout);

		// Create the indirect buffer
		indirect.create(allocator, MaxDrawCommands, MaxInstances);

		// Fill in the draw descriptor set
		auto const indirectBufferInfo = VkDescriptorBufferInfo{
			.buffer = indirect.commandBuffer(),
			.range = VK_WHOLE_SIZE,
		};
		auto const indirectBufferWrite = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = drawDS,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &indirectBufferInfo,
		};
		auto const instanceBufferInfo = VkDescriptorBufferInfo{
			.buffer = indirect.instanceBuffer(),
			.range = VK_WHOLE_SIZE,
		};
		auto const instanceBufferWrite = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = drawDS,
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &instanceBufferInfo,
		};
		auto const indirectBufferWrites = std::array{
			indirectBufferWrite,
			instanceBufferWrite,
		};
		vkUpdateDescriptorSets(device, indirectBufferWrites.size(), indirectBufferWrites.data(), 0, nullptr);
	}

	m_techniques.emplace(id, std::move(result));
}

}
