#include "gfx/world.hpp"

#include "base/zip_view.hpp"
#include "sys/vk/descriptor.hpp"

namespace minote::gfx {

using namespace base;
namespace vk = sys::vk;

void World::create(VkDevice device, VmaAllocator allocator, VkDescriptorPool pool,
	MeshBuffer& meshes) {
	// Create the world descriptor set layout
	m_worldDescriptorSetLayout = vk::createDescriptorSetLayout(device, std::array{
		vk::Descriptor{ // world uniforms
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		vk::Descriptor{ // mesh buffer
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stages = VK_SHADER_STAGE_VERTEX_BIT,
		},
	});

	// Create the world descriptor sets and world uniform buffers
	for (auto[worldDS, worldBuf]: zip_view{m_worldDescriptorSet, m_worldUniforms}) {
		worldDS = vk::allocateDescriptorSet(device, pool, m_worldDescriptorSetLayout);
		worldBuf = vk::createBuffer(allocator, sizeof(Uniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Fill in each world descriptor set
		vk::updateDescriptorSets(device, std::array{
			vk::makeDescriptorSetBufferWrite(worldDS, 0, worldBuf, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			vk::makeDescriptorSetBufferWrite(worldDS, 1, meshes.buffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		});
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
