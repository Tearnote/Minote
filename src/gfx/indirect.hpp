#pragma once

#include <vector>
#include <glm/mat4x4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/buffer.hpp"
#include "gfx/material.hpp"
#include "gfx/mesh.hpp"

namespace minote::gfx {

using namespace base;

struct Instance {

	glm::mat4 transform;
	glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
	glm::vec4 highlight = {0.0f, 0.0f, 0.0f, 0.0f};

};

struct IndirectBuffer {

	struct Command {

		VkDrawIndirectCommand base;
		Material material;
		MaterialData materialData;

	};

	void create(VmaAllocator allocator, size_t maxCommands, size_t maxInstances);

	void destroy(VmaAllocator allocator);

	void enqueue(MeshBuffer::Descriptor const& mesh, std::span<Instance const> instances,
		Material material, MaterialData const& materialData = {});

	auto size() { return m_commandQueue.size(); }

	void reset();

	void upload(VmaAllocator allocator);

	auto commandBuffer() -> sys::vk::Buffer& { return m_commandBuffer; }

	auto instanceBuffer() -> sys::vk::Buffer& { return m_instanceBuffer; }

private:

	sys::vk::Buffer m_commandBuffer = {};
	sys::vk::Buffer m_instanceBuffer = {};
	std::vector<Command> m_commandQueue;
	std::vector<Instance> m_instanceQueue;

};

}
