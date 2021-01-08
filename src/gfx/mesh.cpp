#include "gfx/mesh.hpp"

#include "sys/vk/base.hpp"

namespace minote::gfx {

using namespace base;
using namespace sys;

auto MeshBuffer::addMesh(ID id, std::span<Vertex const> mesh) -> Descriptor {
	ASSERT(!m_buffer.buffer);
	ASSERT(!m_buffer.allocation);
	ASSERT(!m_descriptors.contains(id));

	m_descriptors.emplace(id, Descriptor{
		.vertexOffset = m_vertices.size(),
		.vertexCount = mesh.size(),
	});
	m_vertices.insert(m_vertices.end(), mesh.begin(), mesh.end());

	return getMeshDescriptor(id);
}

void MeshBuffer::upload(VmaAllocator allocator, VkCommandBuffer cmdBuffer, vk::Buffer& staging) {
	ASSERT(!m_buffer.buffer);
	ASSERT(!m_buffer.allocation);

	auto const size = m_vertices.size() * sizeof(Vertex);

	staging = vk::createBuffer(allocator, size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* mappedStorage;
	VK(vmaMapMemory(allocator, staging.allocation, &mappedStorage));
	std::memcpy(mappedStorage, m_vertices.data(), size);
	vmaUnmapMemory(allocator, staging.allocation);

	m_vertices.clear();
	m_vertices.shrink_to_fit();

	m_buffer = vk::createBuffer(allocator, size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	auto bufferCopy = VkBufferCopy{
		.size = size,
	};
	vkCmdCopyBuffer(cmdBuffer, staging.buffer, m_buffer.buffer, 1, &bufferCopy);
}

void MeshBuffer::destroy(VmaAllocator allocator) {
	if (!m_buffer.buffer && !m_buffer.allocation) return;

	vmaDestroyBuffer(allocator, m_buffer.buffer, m_buffer.allocation);
	m_buffer = {};
}

}
