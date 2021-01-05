#pragma once

#include <vector>
#include <glm/mat4x4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/buffer.hpp"
#include "engine/material.hpp"
#include "engine/mesh.hpp"

namespace minote::engine {

struct IndirectBuffer {

	struct Command {

		VkDrawIndirectCommand base;
		Material material;
		MaterialData materialData;

	};

	struct Instance {

		glm::mat4 transform;

	};

	void create(VmaAllocator allocator, base::size_t maxCommands, base::size_t maxInstances);

	void destroy(VmaAllocator allocator);

	void enqueue(MeshBuffer::Descriptor const& mesh, std::span<Instance const> instances,
		Material material, MaterialData const& materialData = {});

	auto size() { return m_commandQueue.size(); }

	void reset();

	void upload(VmaAllocator allocator);

	auto commandBuffer() -> VkBuffer& { return m_commandBuffer.buffer; }

	auto instanceBuffer() -> VkBuffer& { return m_instanceBuffer.buffer; }

private:

	sys::vk::Buffer m_commandBuffer = {};
	sys::vk::Buffer m_instanceBuffer = {};
	std::vector<Command> m_commandQueue;
	std::vector<Instance> m_instanceQueue;

};

}
