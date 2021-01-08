#pragma once

#include <cstddef>
#include <array>
#include <span>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/hashmap.hpp"
#include "base/svector.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "base/id.hpp"
#include "sys/vk/buffer.hpp"

namespace minote::gfx {

struct Vertex {

	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 color;

};

template<base::size_t N>
constexpr auto generateNormals(std::array<Vertex, N> mesh) {
	static_assert(N % 3 == 0);

	base::svector<Vertex, N> result;
	for (auto iv = mesh.begin(), ov = result.begin(); iv != mesh.end(); iv += 3, ov += 3) {
		auto v0 = *(iv + 0);
		auto v1 = *(iv + 1);
		auto v2 = *(iv + 2);

		glm::vec4 const normal = glm::vec4{glm::normalize(glm::cross(glm::vec3{v1.position - v0.position}, glm::vec3{v2.position - v0.position})), 0.0f};
		v0.normal = normal;
		v1.normal = normal;
		v2.normal = normal;
		result.push_back(v0);
		result.push_back(v1);
		result.push_back(v2);
	}
	return result;
}

struct MeshBuffer {

	struct Descriptor {

		base::size_t vertexOffset;
		base::size_t vertexCount;

	};

	auto addMesh(base::ID id, std::span<const Vertex> mesh) -> Descriptor;

	void upload(VmaAllocator allocator, VkCommandBuffer cmdBuffer, sys::vk::Buffer& staging);

	auto getMeshDescriptor(base::ID id) { return m_descriptors.at(id); }

	void destroy(VmaAllocator allocator);

	auto buffer() -> sys::vk::Buffer& { return m_buffer; }

private:

	std::vector<Vertex> m_vertices;
	base::hashmap<base::ID, Descriptor> m_descriptors;
	sys::vk::Buffer m_buffer = {};

};

}
