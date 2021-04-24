#pragma once

#include <string_view>
#include <vector>
#include <span>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "base/hashmap.hpp"
#include "base/types.hpp"
#include "base/id.hpp"

namespace minote::gfx {

using namespace base;

struct Meshes {

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
	std::vector<Descriptor> descriptors;
	hashmap<ID, size_t> descriptorIDs;

	void addGltf(std::string_view name, std::span<char const> mesh);

	[[nodiscard]]
	auto at(ID id) -> Descriptor& { return descriptors[descriptorIDs.at(id)]; }
	[[nodiscard]]
	auto at(ID id) const -> Descriptor const& { return descriptors[descriptorIDs.at(id)]; }

	auto size() const { return descriptors.size(); }

};

}
