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
	
	m_descriptorIDs.emplace(_name, m_descriptors.size());
	auto& desc = m_descriptors.emplace_back(MeshDescriptor{
		.indexOffset = u32(m_indices.size()),
		.indexCount = u32(indexAccessor.count),
		.vertexOffset = u32(m_vertices.size()) });
	
	// Write index data
	
	assert(indexAccessor.component_type == cgltf_component_type_r_16u ||
	       indexAccessor.component_type == cgltf_component_type_r_32u);
	assert(indexAccessor.type == cgltf_type_scalar);
	if (indexAccessor.component_type == cgltf_component_type_r_16u) {
		
		auto* indexTypedBuffer = reinterpret_cast<u16 const*>(indexBuffer);
		m_indices.insert(m_indices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
		
	} else {
		
		auto* indexTypedBuffer = reinterpret_cast<u32 const*>(indexBuffer);
		m_indices.insert(m_indices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
		
	}
	
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
			m_vertices.insert(m_vertices.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
			
		}
		
		// Write vertex normal
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_32f);
			assert(accessor.type == cgltf_type_vec3);
			
			auto* typedBuffer = reinterpret_cast<vec3 const*>(buffer);
			m_normals.insert(m_normals.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
			
		}
		
		// Write vertex color
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "COLOR_0") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_16u);
			assert(accessor.type == cgltf_type_vec4);
			
			auto* typedBuffer = reinterpret_cast<u16vec4 const*>(buffer);
			m_colors.insert(m_colors.end(), typedBuffer, typedBuffer + accessor.count);
			continue;
			
		}
		
		assert(false);
		
	}
	
	L_DEBUG("Parsed GLTF mesh {}", _name);
	
}

auto MeshList::upload(Pool& _pool, vuk::Name _name) && -> MeshBuffer {
	
	ZoneScoped;
	
	auto result = MeshBuffer{
		.vertices = Buffer<vec3>::make(_pool, nameAppend(_name, "vertices"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_vertices, vuk::MemoryUsage::eGPUonly),
		.normals = Buffer<vec3>::make(_pool, nameAppend(_name, "normals"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_normals, vuk::MemoryUsage::eGPUonly),
		.colors = Buffer<u16vec4>::make(_pool, nameAppend(_name, "colors"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_colors, vuk::MemoryUsage::eGPUonly),
		.indices = Buffer<u32>::make(_pool, nameAppend(_name, "indices"),
			vuk::BufferUsageFlagBits::eIndexBuffer |
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_indices, vuk::MemoryUsage::eGPUonly),
		.descriptors = Buffer<MeshDescriptor>::make(_pool, nameAppend(_name, "descriptors"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_descriptors, vuk::MemoryUsage::eGPUonly),
		.cpu_descriptorIDs = std::move(m_descriptorIDs) };
	result.cpu_descriptors = std::move(m_descriptors); // Must still exist for descriptors creation
	
	// Clean up in case this isn't a temporary
	
	m_vertices.clear();
	m_vertices.shrink_to_fit();
	m_normals.clear();
	m_normals.shrink_to_fit();
	m_colors.clear();
	m_colors.shrink_to_fit();
	m_indices.clear();
	m_indices.shrink_to_fit();
	
	return result;
	
	L_DEBUG("Uploaded all meshes to GPU");
	
}

}
