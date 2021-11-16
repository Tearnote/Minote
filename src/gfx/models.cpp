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
	
	L_DEBUG("Loaded model {}", _name);
	
}

auto ModelList::upload(Pool& _pool, vuk::Name _name) && -> ModelBuffer {
	
	ZoneScoped;
	
	auto result = ModelBuffer{
		.vertices = Buffer<VertexType>::make(_pool, nameAppend(_name, "vertices"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_vertices, vuk::MemoryUsage::eGPUonly),
		.normals = Buffer<NormalType>::make(_pool, nameAppend(_name, "normals"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_normals, vuk::MemoryUsage::eGPUonly),
		.colors = Buffer<ColorType>::make(_pool, nameAppend(_name, "colors"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_colors, vuk::MemoryUsage::eGPUonly),
		.indices = Buffer<IndexType>::make(_pool, nameAppend(_name, "indices"),
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
