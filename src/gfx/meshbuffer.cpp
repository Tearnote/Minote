#include "gfx/meshbuffer.hpp"

#include <string_view>
#include <stdexcept>
#include <cassert>
#include <cstring>
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "glm/geometric.hpp"
#include "glm/common.hpp"
#include "glm/vec3.hpp"
#include "fmt/core.h"
#include "base/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

void MeshBuffer::addGltf(std::string_view name, std::span<char const> mesh) {
	auto options = cgltf_options{
		.type = cgltf_file_type_glb,
	};
	auto* gltf = (cgltf_data*)(nullptr);
	if (auto result = cgltf_parse(&options, mesh.data(), mesh.size_bytes(), &gltf); result != cgltf_result_success)
		throw std::runtime_error(fmt::format(R"(Failed to parse mesh "{}": error code {})", name, result));
	defer { cgltf_free(gltf); };
	cgltf_load_buffers(&options, gltf, nullptr);

	assert(gltf->meshes_count == 1);
	assert(gltf->meshes->primitives_count == 1);
	auto& primitive = *gltf->meshes->primitives;

	assert(primitive.indices);
	auto& indexAccessor = *primitive.indices;
	auto* indexBuffer = (char const*)(indexAccessor.buffer_view->buffer->data);
	assert(indexBuffer);
	indexBuffer += indexAccessor.buffer_view->offset;

	descriptorIDs.emplace(name, descriptors.size());
	auto& desc = descriptors.emplace_back(Descriptor{
		.indexOffset = u32(indices.size()),
		.indexCount = u32(indexAccessor.count),
		.vertexOffset = u32(vertices.size()),
	});
	assert(indexAccessor.component_type == cgltf_component_type_r_16u);
	assert(indexAccessor.type == cgltf_type_scalar);
	auto* indexTypedBuffer = (u16*)(indexBuffer);
	indices.insert(indices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);

	for (auto attrIdx = 0_zu; attrIdx < primitive.attributes_count; attrIdx += 1) {
		auto& accessor = *primitive.attributes[attrIdx].data;
		auto* buffer = (char const*)(accessor.buffer_view->buffer->data);
		assert(buffer);
		buffer += accessor.buffer_view->offset;

		if (std::strcmp(primitive.attributes[attrIdx].name, "POSITION") == 0) {
			assert(accessor.component_type == cgltf_component_type_r_32f);
			assert(accessor.type == cgltf_type_vec3);

			// Calculate the furthest point from the origin (for frustum culling)
			auto min = glm::vec3(accessor.min[0], accessor.min[1], accessor.min[2]);
			auto max = glm::vec3(accessor.max[0], accessor.max[1], accessor.max[2]);
			auto far = glm::max(glm::abs(min), glm::abs(max));
			desc.radius = glm::length(far);

			auto* typedBuffer = (glm::vec3*)(buffer);
			vertices.insert(vertices.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
		}

		if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
			assert(accessor.component_type == cgltf_component_type_r_32f);
			assert(accessor.type == cgltf_type_vec3);

			auto* typedBuffer = (glm::vec3*)(buffer);
			normals.insert(normals.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
		}

		if (std::strcmp(primitive.attributes[attrIdx].name, "COLOR_0") == 0) {
			assert(accessor.component_type == cgltf_component_type_r_16u);
			assert(accessor.type == cgltf_type_vec4);

			auto* typedBuffer = (glm::u16vec4*)(buffer);
			colors.insert(colors.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
		}

		assert(false);
	}
}

}
