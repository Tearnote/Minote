#include "gfx/world.hpp"

#include <fmt/core.h>
#include "base/zip_view.hpp"
#include "sys/vk/descriptor.hpp"
#include "sys/vk/debug.hpp"

namespace minote::gfx {

using namespace base;
namespace vk = sys::vk;

void World::create(Context& ctx, MeshBuffer& meshes) {
	// Create the world descriptor set layout
	m_worldDescriptorSetLayout = vk::createDescriptorSetLayout(ctx.device, std::array{
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
		worldDS = vk::allocateDescriptorSet(ctx.device, ctx.descriptorPool, m_worldDescriptorSetLayout);
		worldBuf = vk::createBuffer(ctx.allocator, sizeof(Uniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Fill in each world descriptor set
		vk::updateDescriptorSets(ctx.device, std::array{
			vk::makeDescriptorSetBufferWrite(worldDS, 0, worldBuf, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			vk::makeDescriptorSetBufferWrite(worldDS, 1, meshes.buffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		});
	}
}

void World::destroy(Context& ctx) {
	for (auto& worldBuf: m_worldUniforms)
		vmaDestroyBuffer(ctx.allocator, worldBuf.buffer, worldBuf.allocation);
	vkDestroyDescriptorSetLayout(ctx.device, m_worldDescriptorSetLayout, nullptr);
}

void World::uploadUniforms(Context& ctx, i64 frameIndex) {
	vk::uploadToCpuBuffer(ctx.allocator, m_worldUniforms[frameIndex], uniforms);
}

void World::setDebugName(Context& ctx) {
	vk::setDebugName(ctx.device, m_worldDescriptorSetLayout, "World::m_worldDescriptorSetLayout");
	for (auto[worldDS, worldBuf]: zip_view{m_worldDescriptorSet, m_worldUniforms}) {
		vk::setDebugName(ctx.device, worldDS, fmt::format("World::m_worldDescriptorSet[{}]",
			&worldDS - &m_worldDescriptorSet[0]));
		vk::setDebugName(ctx.device, worldBuf, fmt::format("World::m_worldUniforms[{}]",
			&worldBuf - &m_worldUniforms[0]));
	}
}

}
