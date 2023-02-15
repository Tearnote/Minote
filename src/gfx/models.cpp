#include "gfx/models.hpp"

#include <utility>
#include <cstring>
#include <span>
#include "mpack/mpack.h"
#include "vuk/RenderGraph.hpp"
#include "vuk/Partials.hpp"
#include "gfx/util.hpp"
#include "stx/vector.hpp"
#include "stx/except.hpp"
#include "stx/ranges.hpp"
#include "util/util.hpp"
#include "log.hpp"

namespace minote {

void ModelList::addModel(std::string_view _name, std::span<char const> _model) {
	
	// Load in data
	auto in = mpack_reader_t();
	mpack_reader_init_data(&in, _model.data(), _model.size_bytes());
	
	if (auto magic = mpack_expect_uint(&in); magic != ModelMagic)
		throw stx::runtime_error_fmt("Wrong magic number of model {}: got {}, expected {}", _name, magic, ModelMagic);

	// Process meshes
	mpack_expect_cstr_match(&in, "meshes");
	auto meshCount = mpack_expect_array(&in);
	m_modelIndices.emplace(_name, m_models.size());
	auto& model = m_models.emplace_back();
	model.meshCount = meshCount;
	model.meshOffset = m_meshes.size();
	m_meshes.reserve(meshCount);

	for (auto i: stx::iota(0u, meshCount)) {

		auto& mesh = m_meshes.emplace_back();
		mpack_expect_map_match(&in, 3);

		// Process the material
		mesh.materialIdx = m_materials.size();
		auto& material = m_materials.emplace_back();
		mpack_expect_cstr_match(&in, "material");
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

		// Process the indices
		mpack_expect_cstr_match(&in, "indices");
		auto indexCount = mpack_expect_bin(&in) / sizeof(IndexType);
		mesh.indexCount = indexCount;
		mesh.indexOffset = m_indices.size();
		auto indices = stx::pvector<IndexType>(indexCount);
		mpack_read_bytes(&in, reinterpret_cast<char*>(indices.data()), indexCount * sizeof(IndexType));
		for (auto& v: indices) // Offset values for the unified buffer
			v += m_vertices.size();
		m_indices.insert(m_indices.end(), indices.begin(), indices.end());
		mpack_done_bin(&in);

		// Process the vertices
		mpack_expect_cstr_match(&in, "vertices");
		auto vertexCount = mpack_expect_bin(&in) / sizeof(VertexType);
		auto vertexOffset = m_vertices.size();
		m_vertices.resize(vertexOffset + vertexCount);
		auto* verticesIt = &m_vertices[vertexOffset];
		mpack_read_bytes(&in, reinterpret_cast<char*>(verticesIt), vertexCount * sizeof(VertexType));
		mpack_done_bin(&in);

		mpack_done_map(&in);

	}

	mpack_done_array(&in);
	
	L_DEBUG("Loaded model {} ({} meshes)", _name, meshCount);
	
}

auto ModelList::upload(vuk::Allocator& _allocator) && -> ModelBuffer {
	
	//TODO Change to eTransferQueue pending vuk fix
	auto result = ModelBuffer{
		.materials = vuk::create_buffer_gpu(_allocator,
			vuk::DomainFlagBits::eGraphicsQueue,
			std::span(m_materials)).second,
		.indices = vuk::create_buffer_gpu(_allocator,
			vuk::DomainFlagBits::eGraphicsQueue,
			std::span(m_indices)).second,
		.vertices = vuk::create_buffer_gpu(_allocator,
			vuk::DomainFlagBits::eGraphicsQueue,
			std::span(m_vertices)).second,
		.meshes = vuk::create_buffer_gpu(_allocator,
			vuk::DomainFlagBits::eGraphicsQueue,
			std::span(m_meshes)).second,
		.models = vuk::create_buffer_gpu(_allocator,
			vuk::DomainFlagBits::eGraphicsQueue,
			std::span(m_models)).second,
		.cpu_modelIndices = std::move(m_modelIndices),
	};
	result.cpu_meshes = std::move(m_meshes); // Must still exist for .meshes creation
	result.cpu_models = std::move(m_models);
	
	// Clean up in case this isn't a temporary
	m_materials.clear();
	m_materials.shrink_to_fit();
	m_indices.clear();
	m_indices.shrink_to_fit();
	m_vertices.clear();
	m_vertices.shrink_to_fit();
	
	return result;
	
	L_DEBUG("Uploaded all models to GPU");
	
}

}
