#include "gfx/world.hpp"

#include "base/zip_view.hpp"

namespace minote::gfx {

using namespace base;
namespace vk = sys::vk;

void World::create(VkDevice device, VmaAllocator allocator, VkDescriptorPool pool,
	MeshBuffer& meshes) {
	// Create the world descriptor set layout
	auto const bindings = std::array{
		VkDescriptorSetLayoutBinding{ // world uniforms
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		VkDescriptorSetLayoutBinding{ // mesh buffer
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		},
	};
	auto const layoutCI = VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindings.size(),
		.pBindings = bindings.data(),
	};
	VK(vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_worldDescriptorSetLayout));

	// Create the world descriptor sets and world uniform buffers
	auto const worldDescriptorSetAI = VkDescriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &m_worldDescriptorSetLayout,
	};
	for (auto[worldDS, worldBuf]: zip_view{m_worldDescriptorSet, m_worldUniforms}) {
		VK(vkAllocateDescriptorSets(device, &worldDescriptorSetAI, &worldDS));
		worldBuf = vk::createBuffer(allocator, sizeof(Uniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Fill in each world descriptor set
		auto const worldUniformsInfo = VkDescriptorBufferInfo{
			.buffer = worldBuf.buffer,
			.range = sizeof(Uniforms),
		};
		auto const worldUniformsWrite = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = worldDS,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &worldUniformsInfo,
		};
		auto const meshBufferInfo = VkDescriptorBufferInfo{
			.buffer = meshes.buffer().buffer,
			.range = meshes.buffer().size,
		};
		auto const meshBufferWrite = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = worldDS,
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &meshBufferInfo,
		};
		auto const descriptorWrites = std::array{
			meshBufferWrite,
			worldUniformsWrite,
		};
		vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void World::destroy(VkDevice device, VmaAllocator allocator) {
	for (auto& worldBuf: m_worldUniforms)
		vmaDestroyBuffer(allocator, worldBuf.buffer, worldBuf.allocation);
	vkDestroyDescriptorSetLayout(device, m_worldDescriptorSetLayout, nullptr);
}

void World::uploadUniforms(VmaAllocator allocator, i64 frameIndex) {
	vk::uploadToCpuBuffer(allocator, m_worldUniforms[frameIndex], uniforms);
}

}
