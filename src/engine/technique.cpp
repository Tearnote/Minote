#include "engine/technique.hpp"

#include <utility>
#include "base/zip_view.hpp"
#include "sys/vk/pipeline.hpp"

namespace minote::engine {

using namespace base;
using namespace sys;

constexpr auto defaultVertSrc = std::to_array<u32>({
#include "spv/default.vert.spv"
});

constexpr auto defaultFragSrc = std::to_array<u32>({
#include "spv/default.frag.spv"
});

void TechniqueSet::create(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, MeshBuffer& meshBuffer) {
	// Create the ubershader
	auto const meshBufferBinding = VkDescriptorSetLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
	};
	auto const worldConstantsBinding = VkDescriptorSetLayoutBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	auto const worldBindings = std::to_array<VkDescriptorSetLayoutBinding>({
		meshBufferBinding,
		worldConstantsBinding,
	});
	auto const worldDescriptorSetLayoutCI = VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = worldBindings.size(),
		.pBindings = worldBindings.data(),
	};

	auto const drawCommandBinding = VkDescriptorSetLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	auto const instanceBufferBinding = VkDescriptorSetLayoutBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	auto const drawBindings = std::to_array<VkDescriptorSetLayoutBinding>({
		drawCommandBinding,
		instanceBufferBinding,
	});
	auto const drawDescriptorSetLayoutCI = VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = drawBindings.size(),
		.pBindings = drawBindings.data(),
	};

	auto const descriptorSetLayoutCIs = std::to_array<VkDescriptorSetLayoutCreateInfo const>({
		worldDescriptorSetLayoutCI,
		drawDescriptorSetLayoutCI,
	});
	m_shader = vk::createShader(device, defaultVertSrc, defaultFragSrc, descriptorSetLayoutCIs);

	// Create the world (slot 0) descriptor set
	auto const worldDescriptorSetAI = VkDescriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &m_shader.descriptorSetLayouts[0],
	};
	for (auto[worldDS, worldBuf]: zip_view{m_worldDescriptorSet, m_worldConstants}) {
		VK(vkAllocateDescriptorSets(device, &worldDescriptorSetAI, &worldDS));

		// Create the world data buffer
		worldBuf = vk::createBuffer(allocator, sizeof(World),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Fill in the world descriptor set
		auto const meshBufferInfo = VkDescriptorBufferInfo{
			.buffer = meshBuffer.buffer().buffer,
			.range = meshBuffer.buffer().size,
		};
		auto const meshBufferWrite = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = worldDS,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &meshBufferInfo,
		};
		auto const worldConstantsInfo = VkDescriptorBufferInfo{
			.buffer = worldBuf.buffer,
			.range = sizeof(World),
		};
		auto const worldConstantsWrite = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = worldDS,
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &worldConstantsInfo,
		};
		auto const descriptorWrites = std::to_array<VkWriteDescriptorSet>({
			meshBufferWrite,
			worldConstantsWrite,
		});
		vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	// Create the pipeline layout
	m_pipelineLayout = vk::createPipelineLayout(device, std::to_array({
		m_shader.descriptorSetLayouts[0],
		m_shader.descriptorSetLayouts[1],
	}));
}

void TechniqueSet::destroy(VkDevice device, VmaAllocator allocator) {
	for (auto[id, tech]: m_techniques) {
		for (auto& indirect: tech.indirect)
			indirect.destroy(allocator);
		vkDestroyPipeline(device, tech.pipeline, nullptr);
	}
	vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	for (auto& buf: m_worldConstants)
		vmaDestroyBuffer(allocator, buf.buffer, buf.allocation);
	vk::destroyShader(device, m_shader);
}

void TechniqueSet::addTechnique(base::ID id, VkDevice device, VmaAllocator allocator,
	VkDescriptorPool descriptorPool, VkRenderPass renderPass,
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI,
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI,
	VkPipelineMultisampleStateCreateInfo multisampleStateCI) {
	Technique result;

	// Create the technique's pipeline
	result.pipeline = vk::PipelineBuilder{
		.shaderStageCIs = {
			vk::makePipelineShaderStageCI(VK_SHADER_STAGE_VERTEX_BIT, m_shader.vert),
			vk::makePipelineShaderStageCI(VK_SHADER_STAGE_FRAGMENT_BIT, m_shader.frag),
		},
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = rasterizationStateCI,
		.colorBlendAttachmentState = colorBlendAttachmentState,
		.depthStencilStateCI = depthStencilStateCI,
		.multisampleStateCI = multisampleStateCI,
		.layout = m_pipelineLayout,
	}.build(device, renderPass);

	// Create the technique's draw (slot 1) descriptor set
	auto const drawDescriptorSetAI = VkDescriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &m_shader.descriptorSetLayouts[1],
	};
	for (auto[drawDS, world, sets, indirect]: zip_view{result.drawDescriptorSet,
	                                                   m_worldDescriptorSet,
	                                                   result.descriptorSets,
	                                                   result.indirect}) {
		VK(vkAllocateDescriptorSets(device, &drawDescriptorSetAI, &drawDS));

		sets = {world, drawDS};

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
		auto const indirectBufferWrites = std::to_array<VkWriteDescriptorSet>({
			indirectBufferWrite,
			instanceBufferWrite,
		});
		vkUpdateDescriptorSets(device, indirectBufferWrites.size(), indirectBufferWrites.data(), 0, nullptr);
	}

	m_techniques.emplace(id, std::move(result));
}

}
