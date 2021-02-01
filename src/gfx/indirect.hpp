#pragma once

#include <string_view>
#include <vector>
#include <glm/mat4x4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/buffer.hpp"
#include "gfx/material.hpp"
#include "gfx/context.hpp"
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
		Pass pass;
		Material material;

	};

	void init(Context& ctx, size_t maxCommands, size_t maxInstances);

	void cleanup(Context& ctx);

	void enqueue(MeshBuffer::Descriptor const& mesh, std::span<Instance const> instances,
		Pass pass, Material material = {});

	auto size() { return m_commandQueue.size(); }

	void reset();

	void upload(Context& ctx);

	auto commandBuffer() -> sys::vk::Buffer& { return m_commandBuffer; }

	auto instanceBuffer() -> sys::vk::Buffer& { return m_instanceBuffer; }

	void setDebugName(Context& ctx, std::string_view name) const;

private:

	sys::vk::Buffer m_commandBuffer = {};
	sys::vk::Buffer m_instanceBuffer = {};
	std::vector<Command> m_commandQueue;
	std::vector<Instance> m_instanceQueue;

};

}
