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
	mpack_expect_map_match(&model, 3);
	
	mpack_expect_cstr_match(&model, "format");
	if (auto format = mpack_expect_u32(&model); format != ModelFormat)
		throw runtime_error_fmt("Unexpected model format: got {}, expected {}", format, ModelFormat);
	
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
	
	// Load the meshes
	
	mpack_expect_cstr_match(&model, "meshes");
	auto meshCount = mpack_expect_array(&model);
	m_meshes.reserve(m_meshes.size() + meshCount);
	auto meshList = ivector<u32>();
	meshList.reserve(meshCount);
	
	for (auto i: iota(0u, meshCount)) {
		
		meshList.emplace_back(m_meshes.size());
		auto& mesh = m_meshes.emplace_back();
		mesh.indexOffset = m_indices.size();
		auto vertexOffset = m_vertices.size();
		
		mpack_expect_map_match(&model, 8);
		
		mpack_expect_cstr_match(&model, "transform");
		mpack_expect_array_match(&model, 16);
		for (auto y: iota(0, 4))
		for (auto x: iota(0, 4))
			mesh.transform[x][y] = mpack_expect_float(&model);
		mpack_done_array(&model);
		
		mpack_expect_cstr_match(&model, "materialIdx");
		auto materialIdx = mpack_expect_u32(&model);
		mesh.materialIdx = materialOffset + materialIdx;
		
		mpack_expect_cstr_match(&model, "radius");
		mesh.radius = mpack_expect_float(&model);
		
		mpack_expect_cstr_match(&model, "indexCount");
		auto indexCount = mpack_expect_u32(&model);
		mesh.indexCount = indexCount;
		mpack_expect_cstr_match(&model, "vertexCount");
		auto vertexCount = mpack_expect_u32(&model);
		
		auto indices = pvector<IndexType>(indexCount);
		mpack_expect_cstr_match(&model, "indices");
		mpack_expect_bin_size_buf(&model, reinterpret_cast<char*>(indices.data()), indexCount * sizeof(IndexType));
		for(auto& index : indices)
			index += vertexOffset; // Remove the need to use an offset in shaders
		m_indices.insert(m_indices.end(), indices.begin(), indices.end());
		
		auto verticesSize = m_vertices.size();
		m_vertices.resize(m_vertices.size() + vertexCount);
		auto verticesIt = &m_vertices[verticesSize];
		mpack_expect_cstr_match(&model, "vertices");
		mpack_expect_bin_size_buf(&model, reinterpret_cast<char*>(verticesIt), vertexCount * sizeof(VertexType));
		
		auto normalsSize = m_normals.size();
		m_normals.resize(m_normals.size() + vertexCount);
		auto normalsIt = &m_normals[normalsSize];
		mpack_expect_cstr_match(&model, "normals");
		mpack_expect_bin_size_buf(&model, reinterpret_cast<char*>(normalsIt), vertexCount * sizeof(NormalType));
		
		mpack_done_map(&model);
		
	}
	mpack_done_array(&model);
	m_modelMeshes.emplace(_name, std::move(meshList));
	
	mpack_done_map(&model);
	mpack_reader_destroy(&model);
	
	L_DEBUG("Loaded model {}: {} materials, {} meshes", _name, materialCount, meshCount);
	
}

auto ModelList::upload(Pool& _pool, vuk::Name _name) && -> ModelBuffer {
	
	ZoneScoped;
	
	auto result = ModelBuffer{
		.materials = Buffer<Material>::make(_pool, nameAppend(_name, "materials"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_materials),
		.indices = Buffer<IndexType>::make(_pool, nameAppend(_name, "indices"),
			vuk::BufferUsageFlagBits::eIndexBuffer |
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_indices),
		.vertices = Buffer<VertexType>::make(_pool, nameAppend(_name, "vertices"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_vertices),
		.normals = Buffer<NormalType>::make(_pool, nameAppend(_name, "normals"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_normals),
		.meshes = Buffer<Mesh>::make(_pool, nameAppend(_name, "meshes"),
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_meshes),
		.cpu_modelMeshes = std::move(m_modelMeshes) };
	result.cpu_meshes = std::move(m_meshes); // Must still exist for .meshes creation
	
	// Clean up in case this isn't a temporary
	
	m_materials.clear();
	m_materials.shrink_to_fit();
	m_indices.clear();
	m_indices.shrink_to_fit();
	m_vertices.clear();
	m_vertices.shrink_to_fit();
	m_normals.clear();
	m_normals.shrink_to_fit();
	
	return result;
	
	L_DEBUG("Uploaded all models to GPU");
	
}

}
