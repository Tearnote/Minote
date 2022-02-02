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
	
	// Load model data
	
	auto model = mpack_reader_t();
	mpack_reader_init_data(&model, _model.data(), _model.size_bytes());
	
	if (auto magic = mpack_expect_u32(&model); magic != ModelMagic)
		throw runtime_error_fmt("Wrong magic number of model {}: got {}, expected {}", _name, magic, ModelMagic);
	
	mpack_expect_map_match(&model, 2);
	
	// Load the materials
	
	auto materialOffset = m_materials.size();
	
	mpack_expect_cstr_match(&model, "materials");
	auto materialCount = mpack_expect_array(&model);
	m_materials.reserve(m_materials.size() + materialCount);
	for (auto i: iota(0u, materialCount)) {
		
		auto& material = m_materials.emplace_back();
		material.id = +MaterialType::PBR;
		
		mpack_expect_map_match(&model, 4);
		
		mpack_expect_cstr_match(&model, "color");
		mpack_expect_array_match(&model, 4);
		material.color[0] = mpack_expect_float(&model);
		material.color[1] = mpack_expect_float(&model);
		material.color[2] = mpack_expect_float(&model);
		material.color[3] = mpack_expect_float(&model);
		mpack_done_array(&model);
		
		mpack_expect_cstr_match(&model, "emissive");
		mpack_expect_array_match(&model, 3);
		material.emissive[0] = mpack_expect_float(&model);
		material.emissive[1] = mpack_expect_float(&model);
		material.emissive[2] = mpack_expect_float(&model);
		mpack_done_array(&model);
		
		mpack_expect_cstr_match(&model, "metalness");
		material.metalness = mpack_expect_float(&model);
		mpack_expect_cstr_match(&model, "roughness");
		material.roughness = mpack_expect_float(&model);
		
		mpack_done_map(&model);
		
	}
	mpack_done_array(&model);
	
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
	
	// Load the meshlets from the meshes
	
	m_modelIndices.emplace(_name, m_models.size());
	auto& modelDesc = m_models.emplace_back(Model{
		.meshletOffset = u32(m_meshlets.size()),
		.meshletCount = 0 });
	
	mpack_expect_cstr_match(&model, "meshes");
	auto meshCount = mpack_expect_array(&model);
	for (auto i: iota(0u, meshCount)) {
		
		mpack_expect_map_match(&model, 6);
		
		mpack_expect_cstr_match(&model, "materialIdx");
		auto materialIdx = mpack_expect_u32(&model);
		
		mpack_expect_cstr_match(&model, "meshlets");
		auto meshletCount = mpack_expect_array(&model);
		modelDesc.meshletCount += meshletCount;
		for (auto j: iota(0u, meshletCount)) {
			
			mpack_expect_map_match(&model, 5);
			
			auto& meshlet = m_meshlets.emplace_back();
			meshlet.materialIdx = materialOffset + materialIdx;
			meshlet.vertexDataOffset = m_vertices.size();
			
			mpack_expect_cstr_match(&model, "indexOffset");
			auto indexOffset = mpack_expect_u32(&model);
			meshlet.indexOffset = indexOffset + m_triIndices.size();
			
			mpack_expect_cstr_match(&model, "indexCount");
			auto indexCount = mpack_expect_u32(&model);
			meshlet.indexCount = indexCount;
			
			mpack_expect_cstr_match(&model, "vertexOffset");
			auto vertexOffset = mpack_expect_u32(&model);
			meshlet.vertexOffset = vertexOffset + m_vertIndices.size();
			
			mpack_expect_cstr_match(&model, "boundingSphereCenter");
			mpack_expect_array_match(&model, 3);
			for (auto i: iota(0, 3))
				meshlet.boundingSphereCenter[i] = mpack_expect_float(&model);
			mpack_done_array(&model);
			
			mpack_expect_cstr_match(&model, "boundingSphereRadius");
			meshlet.boundingSphereRadius = mpack_expect_float(&model);
			
			mpack_done_map(&model);
			
		}
		mpack_done_array(&model);
		
		mpack_expect_cstr_match(&model, "triIndices");
		auto triIndexCount = mpack_expect_bin(&model) / sizeof(TriIndexType);
		auto triIndices = pvector<TriIndexType>(triIndexCount);
		mpack_read_bytes(&model, reinterpret_cast<char*>(triIndices.data()), triIndexCount * sizeof(TriIndexType));
		m_triIndices.insert(m_triIndices.end(), triIndices.begin(), triIndices.end());
		mpack_done_bin(&model);
		
		mpack_expect_cstr_match(&model, "vertIndices");
		auto vertIndexCount = mpack_expect_bin(&model) / sizeof(VertIndexType);
		auto vertIndexOffset = m_vertIndices.size();
		m_vertIndices.resize(vertIndexOffset + vertIndexCount);
		auto* vertIndicesIt = &m_vertIndices[vertIndexOffset];
		mpack_read_bytes(&model, reinterpret_cast<char*>(vertIndicesIt), vertIndexCount * sizeof(VertIndexType));
		mpack_done_bin(&model);
		
		mpack_expect_cstr_match(&model, "vertices");
		auto vertexCount = mpack_expect_bin(&model) / sizeof(VertexType);
		auto vertexOffset = m_vertices.size();
		m_vertices.resize(vertexOffset + vertexCount);
		auto* verticesIt = &m_vertices[vertexOffset];
		mpack_read_bytes(&model, reinterpret_cast<char*>(verticesIt), vertexCount * sizeof(VertexType));
		mpack_done_bin(&model);
		
		mpack_expect_cstr_match(&model, "normals");
		auto normalCount = mpack_expect_bin(&model) / sizeof(NormalType);
		auto normalOffset = m_normals.size();
		m_normals.resize(normalOffset + normalCount);
		auto* normalsIt = &m_normals[normalOffset];
		mpack_read_bytes(&model, reinterpret_cast<char*>(normalsIt), normalCount * sizeof(NormalType));
		mpack_done_bin(&model);
		
		mpack_done_map(&model);
		
	}
	mpack_done_array(&model);
	
	mpack_done_map(&model);
	mpack_reader_destroy(&model);
	
	L_DEBUG("Loaded model {}: {} materials, {} meshlets", _name, materialCount, modelDesc.meshletCount);
	
}

auto ModelList::upload(Pool& _pool, vuk::Name _name) && -> ModelBuffer {
	
	ZoneScoped;
	
	auto result = ModelBuffer{
		.materials = Buffer<Material>::make(_pool, nameAppend(_name, "materials"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_materials),
		.triIndices = Buffer<u32>::make(_pool, nameAppend(_name, "triIndices"),
			vuk::BufferUsageFlagBits::eIndexBuffer |
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

}
