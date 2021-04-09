#include "gfx/meshbuffer.hpp"

#include <cassert>

namespace minote::gfx {

using namespace base;

void MeshBuffer::addMesh(ID name, std::span<glm::vec3 const> _vertices, std::span<glm::vec3 const> _normals,
	std::span<glm::u8vec4 const> _colors, std::span<std::array<u32, 3> const> _indexes) {
	assert((_vertices.size() == _normals.size()) && (_vertices.size() == _colors.size()));

	descriptors.emplace(name, Descriptor{
		.indexOffset = u32(indices.size()),
		.indexCount = u32(_indexes.size() * 3),
		.vertexOffset = u32(vertices.size()),
	});

	vertices.insert(vertices.end(), _vertices.begin(), _vertices.end());
	normals.insert(normals.end(), _normals.begin(), _normals.end());
	colors.insert(colors.end(), _colors.begin(), _colors.end());
	indices.insert(indices.end(), _indexes.begin(), _indexes.end());
}

auto MeshBuffer::makeBuffers() -> std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
	std::vector<glm::u8vec4>, std::vector<std::array<u32, 3>>> {
	return std::make_tuple(std::move(vertices), std::move(normals), std::move(colors), std::move(indices));
}

}
