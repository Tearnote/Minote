#pragma once

#include <array>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "vuk/CommandBuffer.hpp"
#include "base/svector.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

struct Vertex {

	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 color;

	static auto format() -> vuk::Packed {
		return vuk::Packed{
			vuk::Format::eR32G32B32A32Sfloat,
			vuk::Format::eR32G32B32A32Sfloat,
			vuk::Format::eR32G32B32A32Sfloat,
		};
	}

};

template<size_t N>
constexpr auto generateNormals(std::array<Vertex, N> mesh) {
	static_assert(N % 3 == 0);

	svector<Vertex, N> result;
	for (auto iv = mesh.begin(), ov = result.begin(); iv != mesh.end(); iv += 3, ov += 3) {
		auto v0 = *(iv + 0);
		auto v1 = *(iv + 1);
		auto v2 = *(iv + 2);

		auto normal = glm::vec4(glm::normalize(glm::cross(glm::vec3(v1.position - v0.position), glm::vec3(v2.position - v0.position))), 0.0f);
		v0.normal = normal;
		v1.normal = normal;
		v2.normal = normal;
		result.push_back(v0);
		result.push_back(v1);
		result.push_back(v2);
	}
	return result;
}

}
