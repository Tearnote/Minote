#include "gfx/models.hpp"

#include <utility>
#include <cassert>
#include <cstring>
#include "mpack/mpack.h"
#include "gfx/util.hpp"
#include "util/error.hpp"
#include "util/util.hpp"
#include "util/log.hpp"

namespace minote {

void ModelList::addModel(string_view _name, std::span<char const> _model) {
	
	// Load in data
	
	auto in = mpack_reader_t();
	mpack_reader_init_data(&in, _model.data(), _model.size_bytes());
	
	if (auto magic = mpack_expect_u32(&in); magic != ModelMagic)
		throw runtime_error_fmt("Wrong magic number of model {}: got {}, expected {}", _name, magic, ModelMagic);
	
	mpack_expect_map_match(&in, 6);
	
	// Load the materials
	
	auto materialOffset = m_materials.size();
	
	mpack_expect_cstr_match(&in, "materials");
	auto materialCount = mpack_expect_array(&in);
	m_materials.reserve(m_materials.size() + materialCount);
	for (auto i: iota(0u, materialCount)) {
		
		auto& material = m_materials.emplace_back();
		material.id = +MaterialType::PBR;
		
		mpack_expect_map_match(&in, 4);
		
		mpack_expect_cstr_match(&in, "color");
		mpack_expect_array_match(&in, 4);
		material.color[0] = mpack_expect_float(&in);
		material.color[1] = mpack_expect_float(&in);
		material.color[2] = mpack_expect_float(&in);
		material.color[3] = mpack_expect_float(&in);
		mpack_done_array(&in);
		
		mpack_expect_cstr_match(&in, "emissive");
		mpack_expect_array_match(&in, 3);
		material.emissive[0] = mpack_expect_float(&in);
		material.emissive[1] = mpack_expect_float(&in);
		material.emissive[2] = mpack_expect_float(&in);
		mpack_done_array(&in);
		
		mpack_expect_cstr_match(&in, "metalness");
		material.metalness = mpack_expect_float(&in);
		mpack_expect_cstr_match(&in, "roughness");
		material.roughness = mpack_expect_float(&in);
		
		mpack_done_map(&in);
		
	}
	mpack_done_array(&in);
	
	// Safety fallback
	if (materialCount == 0) {
		
		auto& material = m_materials.emplace_back();
		material.id = +MaterialType::PBR;
		material.color = {1.0f, 1.0f, 1.0f, 1.0f};
		material.emissive = {0.0f, 0.0f, 0.0f};
		material.metalness = 0.0f;
		material.roughness = 0.0f;
		L_WARN("Model {} has no materials, using defaults", _name);
		
	}
	
	// Load the meshlets
	
	m_modelIndices.emplace(_name, m_models.size());
	auto& model = m_models.emplace_back(Model{
		.meshletOffset = u32(m_meshlets.size()),
		.meshletCount = 0 });
	
	mpack_expect_cstr_match(&in, "meshlets");
	auto meshletCount = mpack_expect_array(&in);
	model.meshletCount += meshletCount;
	for (auto i: iota(0u, meshletCount)) {
		
		mpack_expect_map_match(&in, 8);
		
		auto& meshlet = m_meshlets.emplace_back();
		auto& aabb = m_meshletAABBs.emplace_back();
		
		mpack_expect_cstr_match(&in, "materialIdx");
		auto materialIdx = mpack_expect_u32(&in);
		meshlet.materialIdx = materialOffset + materialIdx;
		
		mpack_expect_cstr_match(&in, "indexOffset");
		auto indexOffset = mpack_expect_u32(&in);
		meshlet.indexOffset = indexOffset + m_triIndices.size();
		
		mpack_expect_cstr_match(&in, "indexCount");
		auto indexCount = mpack_expect_u32(&in);
		meshlet.indexCount = indexCount;
		
		mpack_expect_cstr_match(&in, "vertexOffset");
		auto vertexOffset = mpack_expect_u32(&in);
		meshlet.vertexOffset = vertexOffset + m_vertIndices.size();
		
		mpack_expect_cstr_match(&in, "boundingSphereCenter");
		mpack_expect_array_match(&in, 3);
		for (auto i: iota(0, 3))
			meshlet.boundingSphereCenter[i] = mpack_expect_float(&in);
		mpack_done_array(&in);
		
		mpack_expect_cstr_match(&in, "boundingSphereRadius");
		meshlet.boundingSphereRadius = mpack_expect_float(&in);
		
		mpack_expect_cstr_match(&in, "aabbMin");
		mpack_expect_array_match(&in, 3);
		for (auto i: iota(0, 3))
			aabb.min[i] = mpack_expect_float(&in);
		mpack_done_array(&in);
		
		mpack_expect_cstr_match(&in, "aabbMax");
		mpack_expect_array_match(&in, 3);
		for (auto i: iota(0, 3))
			aabb.max[i] = mpack_expect_float(&in);
		mpack_done_array(&in);
		
		mpack_done_map(&in);
		
	}
	mpack_done_array(&in);
	
	mpack_expect_cstr_match(&in, "triIndices");
	auto triIndexCount = mpack_expect_bin(&in) / sizeof(TriIndexType);
	auto triIndices = pvector<TriIndexType>(triIndexCount);
	mpack_read_bytes(&in, reinterpret_cast<char*>(triIndices.data()), triIndexCount * sizeof(TriIndexType));
	m_triIndices.insert(m_triIndices.end(), triIndices.begin(), triIndices.end());
	mpack_done_bin(&in);
	
	mpack_expect_cstr_match(&in, "vertIndices");
	auto vertIndexCount = mpack_expect_bin(&in) / sizeof(VertIndexType);
	auto vertIndices = pvector<VertIndexType>(vertIndexCount);
	mpack_read_bytes(&in, reinterpret_cast<char*>(vertIndices.data()), vertIndexCount * sizeof(VertIndexType));
	for (auto& v: vertIndices) // Offset values for the unified buffer
		v += m_vertices.size();
	m_vertIndices.insert(m_vertIndices.end(), vertIndices.begin(), vertIndices.end());
	mpack_done_bin(&in);
	
	mpack_expect_cstr_match(&in, "vertices");
	auto vertexCount = mpack_expect_bin(&in) / sizeof(VertexType);
	auto vertexOffset = m_vertices.size();
	m_vertices.resize(vertexOffset + vertexCount);
	auto* verticesIt = &m_vertices[vertexOffset];
	mpack_read_bytes(&in, reinterpret_cast<char*>(verticesIt), vertexCount * sizeof(VertexType));
	mpack_done_bin(&in);
	
	mpack_expect_cstr_match(&in, "normals");
	auto normalCount = mpack_expect_bin(&in) / sizeof(NormalType);
	auto normalOffset = m_normals.size();
	m_normals.resize(normalOffset + normalCount);
	auto* normalsIt = &m_normals[normalOffset];
	mpack_read_bytes(&in, reinterpret_cast<char*>(normalsIt), normalCount * sizeof(NormalType));
	mpack_done_bin(&in);
	
	mpack_done_map(&in);
	mpack_reader_destroy(&in);
	
	L_DEBUG("Loaded model {}: {} materials, {} meshlets", _name, materialCount, model.meshletCount);
	
}
/*
auto ModelList::upload(Pool& _pool, vuk::Name _name) && -> ModelBuffer {
	
	auto result = ModelBuffer{
		.materials = Buffer<Material>::make(_pool, nameAppend(_name, "materials"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_materials),
		.triIndices = Buffer<u32>::make(_pool, nameAppend(_name, "triIndices"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_triIndices),
		.vertIndices = Buffer<VertIndexType>::make(_pool, nameAppend(_name, "vertIndices"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_vertIndices),
		.vertices = Buffer<VertexType>::make(_pool, nameAppend(_name, "vertices"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_vertices),
		.normals = Buffer<NormalType>::make(_pool, nameAppend(_name, "normals"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_normals),
		.meshlets = Buffer<Meshlet>::make(_pool, nameAppend(_name, "meshlets"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_meshlets),
		.models = Buffer<Model>::make(_pool, nameAppend(_name, "models"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_models),
		.cpu_modelIndices = std::move(m_modelIndices) };
	result.cpu_meshlets = std::move(m_meshlets); // Must still exist for .meshlets creation
	result.cpu_meshletAABBs = std::move(m_meshletAABBs);
	result.cpu_models = std::move(m_models);
	
	// Clean up in case this isn't a temporary
	
	m_materials.clear();
	m_materials.shrink_to_fit();
	m_triIndices.clear();
	m_triIndices.shrink_to_fit();
	m_vertIndices.clear();
	m_vertIndices.shrink_to_fit();
	m_vertices.clear();
	m_vertices.shrink_to_fit();
	m_normals.clear();
	m_normals.shrink_to_fit();
	
	return result;
	
	L_DEBUG("Uploaded all models to GPU");
	
}
*/
}
