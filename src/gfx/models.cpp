#include "gfx/models.hpp"

#include <utility>
#include <cassert>
#include <cstring>
#include "Tracy.hpp"
#include "mpack/mpack.h"
#include "gfx/util.hpp"
#include "base/error.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "tools/modelSchema.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;
using namespace tools;

void ModelList::addModel(string_view _name, std::span<char const> _model) {
	
	ZoneScoped;
	
	// Insert model descriptor
	
	m_descriptorIDs.emplace(_name, m_descriptors.size());
	auto& desc = m_descriptors.emplace_back(ModelDescriptor{
		.indexOffset = u32(m_indices.size()),
		.vertexOffset = u32(m_vertices.size()) });
	
	// Load model data
	
	auto modelReader = mpack_reader_t();
	mpack_reader_init_data(&modelReader, _model.data(), _model.size_bytes());
	
	mpack_expect_map_match(&modelReader, ModelElements);
	mpack_expect_cstr_match(&modelReader, "format");
	if (auto format = mpack_expect_u32(&modelReader); format != ModelFormat)
		throw runtime_error_fmt("Unexpected model format: got {}, expected {}", format, ModelFormat);
	
	mpack_expect_cstr_match(&modelReader, "indexCount");
	auto indexCount = mpack_expect_u32(&modelReader);
	mpack_expect_cstr_match(&modelReader, "vertexCount");
	auto vertexCount = mpack_expect_u32(&modelReader);
	
	auto indicesSize = m_indices.size();
	m_indices.resize(m_indices.size() + indexCount);
	auto indicesIt = &m_indices[indicesSize];
	mpack_expect_cstr_match(&modelReader, "indices");
	mpack_expect_bin_size_buf(&modelReader, reinterpret_cast<char*>(indicesIt), indexCount * sizeof(IndexType));
	
	auto verticesSize = m_vertices.size();
	m_vertices.resize(m_vertices.size() + vertexCount);
	auto verticesIt = &m_vertices[verticesSize];
	mpack_expect_cstr_match(&modelReader, "vertices");
	mpack_expect_bin_size_buf(&modelReader, reinterpret_cast<char*>(verticesIt), vertexCount * sizeof(VertexType));
	
	auto normalsSize = m_normals.size();
	m_normals.resize(m_normals.size() + vertexCount);
	auto normalsIt = &m_normals[normalsSize];
	mpack_expect_cstr_match(&modelReader, "normals");
	mpack_expect_bin_size_buf(&modelReader, reinterpret_cast<char*>(normalsIt), vertexCount * sizeof(NormalType));
	
	auto colorsSize = m_colors.size();
	m_colors.resize(m_colors.size() + vertexCount);
	auto colorsIt = &m_colors[colorsSize];
	mpack_expect_cstr_match(&modelReader, "colors");
	mpack_expect_bin_size_buf(&modelReader, reinterpret_cast<char*>(colorsIt), vertexCount * sizeof(ColorType));
	
	mpack_expect_cstr_match(&modelReader, "radius");
	auto radius = mpack_expect_float(&modelReader);
	
	mpack_done_map(&modelReader);
	mpack_reader_destroy(&modelReader);
	
	desc.indexCount = indexCount;
	desc.radius = radius;
	
	/*
	
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
	*/
	L_DEBUG("Loaded model {}", _name);
	
}

auto ModelList::upload(Pool& _pool, vuk::Name _name) && -> ModelBuffer {
	
	ZoneScoped;
	
	auto result = ModelBuffer{
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
		.descriptors = Buffer<ModelDescriptor>::make(_pool, nameAppend(_name, "descriptors"),
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
	
	L_DEBUG("Uploaded all models to GPU");
	
}

}
