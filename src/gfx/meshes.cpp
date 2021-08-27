#include "gfx/meshes.hpp"

#include <utility>
#include <cassert>
#include <cstring>
#include "Tracy.hpp"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "quill/Fmt.h"
#include "gfx/util.hpp"
#include "base/error.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

void MeshList::addGltf(string_view _name, std::span<char const> _mesh) {
	
	ZoneScoped;
	
	// Load and parse
	
	auto options = cgltf_options{ .type = cgltf_file_type_glb };
	auto* gltf = static_cast<cgltf_data*>(nullptr);
	if (auto result = cgltf_parse(&options, _mesh.data(), _mesh.size_bytes(), &gltf); result != cgltf_result_success)
		throw runtime_error_fmt(R"(Failed to parse mesh "{}": error code {})", _name, result);
	defer { cgltf_free(gltf); };
	cgltf_load_buffers(&options, gltf, nullptr);
	
	// Choose mesh and primitive
	
	assert(gltf->meshes_count == 1);
	assert(gltf->meshes->primitives_count == 1);
	auto& primitive = *gltf->meshes->primitives;
	
	// Fetch index data
	
	assert(primitive.indices);
	auto& indexAccessor = *primitive.indices;
	auto* indexBuffer = static_cast<char const*>(indexAccessor.buffer_view->buffer->data);
	assert(indexBuffer);
	indexBuffer += indexAccessor.buffer_view->offset;
	
	// Write mesh descriptor
	
	descriptorIDs.emplace(_name, descriptors.size());
	auto& desc = descriptors.emplace_back(MeshDescriptor{
		.indexOffset = u32(indices.size()),
		.indexCount = u32(indexAccessor.count),
		.vertexOffset = u32(vertices.size()) });
	
	// Write index data
	
	assert(indexAccessor.component_type == cgltf_component_type_r_16u);
	assert(indexAccessor.type == cgltf_type_scalar);
	auto* indexTypedBuffer = reinterpret_cast<u16 const*>(indexBuffer);
	indices.insert(indices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
	
	// Fetch all vertex attributes
	
	for (auto attrIdx: iota(0_zu, primitive.attributes_count)) {
		
		auto& accessor = *primitive.attributes[attrIdx].data;
		auto* buffer = static_cast<char const*>(accessor.buffer_view->buffer->data);
		assert(buffer);
		buffer += accessor.buffer_view->offset;
		
		// Write vertex position
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "POSITION") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_32f);
			assert(accessor.type == cgltf_type_vec3);
			
			// Calculate the AABB and furthest point from the origin
			auto aabbMin = vec3{accessor.min[0], accessor.min[1], accessor.min[2]};
			auto aabbMax = vec3{accessor.max[0], accessor.max[1], accessor.max[2]};
			auto pfar = max(abs(aabbMin), abs(aabbMax));
			desc.radius = length(pfar);
			
			auto* typedBuffer = reinterpret_cast<vec3 const*>(buffer);
			vertices.insert(vertices.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
			
		}
		
		// Write vertex normal
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_32f);
			assert(accessor.type == cgltf_type_vec3);
			
			auto* typedBuffer = reinterpret_cast<vec3 const*>(buffer);
			normals.insert(normals.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
			
		}
		
		// Write vertex color
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "COLOR_0") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_16u);
			assert(accessor.type == cgltf_type_vec4);
			
			auto* typedBuffer = reinterpret_cast<u16vec4 const*>(buffer);
			colors.insert(colors.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
			
		}
		
		assert(false);
		
	}
	
	L_DEBUG("Parsed GLTF mesh {}", _name);
	
}

auto MeshList::upload(vuk::PerThreadContext& _ptc, vuk::Name _name) && -> MeshBuffer {
	
	ZoneScoped;
	
	auto result = MeshBuffer{
		.verticesBuf = Buffer<vec3>(_ptc, nameAppend(_name, "vertices"), vertices,
			vuk::BufferUsageFlagBits::eVertexBuffer, vuk::MemoryUsage::eGPUonly),
		.normalsBuf = Buffer<vec3>(_ptc, nameAppend(_name, "normals"), normals,
			vuk::BufferUsageFlagBits::eVertexBuffer, vuk::MemoryUsage::eGPUonly),
		.colorsBuf = Buffer<u16vec4>(_ptc, nameAppend(_name, "colors"), colors,
			vuk::BufferUsageFlagBits::eVertexBuffer, vuk::MemoryUsage::eGPUonly),
		.indicesBuf = Buffer<u16>(_ptc, nameAppend(_name, "indices"), indices,
			vuk::BufferUsageFlagBits::eIndexBuffer, vuk::MemoryUsage::eGPUonly),
		.descriptorBuf = Buffer<MeshDescriptor>(_ptc, nameAppend(_name, "descriptors"), descriptors,
			vuk::BufferUsageFlagBits::eStorageBuffer, vuk::MemoryUsage::eGPUonly),
		.descriptorIDs = std::move(descriptorIDs) };
	result.descriptors = std::move(descriptors); // Must still exist for descriptorBuf creation
	
	// Clean up in case this isn't a temporary
	
	vertices.clear();
	vertices.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();
	colors.clear();
	colors.shrink_to_fit();
	indices.clear();
	indices.shrink_to_fit();
	
	return result;
	
	L_DEBUG("Uploaded all meshes to GPU");
	
}

}
