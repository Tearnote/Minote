#include "gfx/indirect.hpp"

#include "VulkanMemoryAllocator/vma.h"
#include "sys/vk/buffer.hpp"
#include "base/assert.hpp"

namespace minote::gfx {

using namespace base;
using namespace sys;

void IndirectBuffer::create(VmaAllocator allocator, size_t maxCommands, size_t maxInstances) {
	ASSERT(!m_commandBuffer.buffer && !m_commandBuffer.allocation);
	ASSERT(!m_instanceBuffer.buffer && !m_instanceBuffer.allocation);

	m_commandQueue.reserve(maxCommands);
	m_instanceQueue.reserve(maxInstances);
	m_commandBuffer = vk::createBuffer(allocator, maxCommands * sizeof(Command),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	m_instanceBuffer = vk::createBuffer(allocator, maxInstances * sizeof(Instance),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void IndirectBuffer::destroy(VmaAllocator allocator) {
	if (!m_commandBuffer.buffer || !m_commandBuffer.allocation) return;

	vmaDestroyBuffer(allocator, m_instanceBuffer.buffer, m_instanceBuffer.allocation);
	vmaDestroyBuffer(allocator, m_commandBuffer.buffer, m_commandBuffer.allocation);
	m_instanceBuffer = {};
	m_commandBuffer = {};
	m_instanceQueue.clear();
	m_instanceQueue.shrink_to_fit();
	m_commandQueue.clear();
	m_commandQueue.shrink_to_fit();
}

void IndirectBuffer::enqueue(MeshBuffer::Descriptor const& mesh, std::span<Instance const> instances,
	Material material, MaterialData const& materialData) {
	ASSERT(m_commandBuffer.buffer && m_commandBuffer.allocation);
	ASSERT(m_instanceBuffer.buffer && m_instanceBuffer.allocation);

	if (m_commandQueue.size() == m_commandQueue.capacity())
		throw std::runtime_error{fmt::format("Maximum number of draw commands reached: {}", m_commandQueue.capacity())};
	if (m_instanceQueue.size() + instances.size() > m_instanceQueue.capacity())
		throw std::runtime_error{fmt::format("Maximum number of instances reached: {}", m_instanceQueue.capacity())};

	m_commandQueue.emplace_back(Command{
		.base = {
			.vertexCount = static_cast<u32>(mesh.vertexCount),
			.instanceCount = static_cast<u32>(instances.size()),
			.firstVertex = static_cast<u32>(mesh.vertexOffset),
			.firstInstance = static_cast<u32>(m_instanceQueue.size()),
		},
		.material = material,
		.materialData = materialData,
	});
	m_instanceQueue.insert(m_instanceQueue.end(), instances.begin(), instances.end());
}

void IndirectBuffer::reset() {
	m_commandQueue.clear();
	m_instanceQueue.clear();
}

void IndirectBuffer::upload(VmaAllocator allocator) {
	ASSERT(m_commandBuffer.buffer && m_commandBuffer.allocation);
	ASSERT(m_instanceBuffer.buffer && m_instanceBuffer.allocation);

	vk::uploadToCpuBuffer<Command>(allocator, m_commandBuffer, m_commandQueue);
	vk::uploadToCpuBuffer<Instance>(allocator, m_instanceBuffer, m_instanceQueue);
}

}
