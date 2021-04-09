#pragma once

#include <string_view>
#include <vector>
#include <array>
#include <tuple>
#include <span>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "base/hashmap.hpp"
#include "base/types.hpp"
#include "base/id.hpp"

namespace minote::gfx {

using namespace base;

struct MeshBuffer {

	struct Descriptor {

		u32 indexOffset;
		u32 indexCount;
		u32 vertexOffset;

	};

	void addMesh(ID name, std::span<glm::vec3 const> vertices, std::span<glm::vec3 const> normals,
		std::span<glm::u8vec4 const> colors, std::span<std::array<u32, 3> const> indices);

	[[nodiscard]]
	auto getDescriptor(ID name) const -> Descriptor { return descriptors.at(name); }

	auto makeBuffers() -> std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
	    std::vector<glm::u8vec4>, std::vector<std::array<u32, 3>>>;

private:

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::u8vec4> colors;
	std::vector<std::array<u32, 3>> indices;

	hashmap<ID, Descriptor> descriptors;

};

}
