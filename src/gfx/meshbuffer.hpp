#pragma once

#include <string_view>
#include <vector>
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

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::u16vec4> colors;
	std::vector<u16> indices;

	struct Descriptor {

		u32 indexOffset;
		u32 indexCount;
		u32 vertexOffset;
		f32 radius;

	};
	hashmap<ID, Descriptor> descriptors;

	void addGltf(std::string_view name, std::span<char const> mesh);

};

}
