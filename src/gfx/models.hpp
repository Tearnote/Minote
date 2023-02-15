#pragma once

#include <string_view>
#include <type_traits>
#include <span>
#include "vuk/Future.hpp"
#include "gfx/resource.hpp"
#include "stx/hashmap.hpp"
#include "util/vector.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/id.hpp"
#include "tools/modelSchema.hpp"

namespace minote {

struct Material {
	float4 color;
	float3 emissive;
	//TODO compress metalness and roughness into a single 32bit value
	float metalness;
	float roughness;
	float _pad0;
	float _pad1;
	float _pad2;
};

struct Mesh {
	// Index+size into ModelBuffer::indices
	uint indexOffset;
	uint indexCount;
	
	uint materialIdx; // Index into ModelBuffer::materials
	uint _pad0;
};

// Index+size into ModelBuffer::meshes
struct Model {
	uint meshOffset;
	uint meshCount;
};

// A set of buffers storing vertex data for all models, and how to access each
// model within the buffer
struct ModelBuffer {
	Buffer<Material> materials;
	Buffer<IndexType> indices;
	Buffer<VertexType> vertices;
	
	Buffer<Mesh> meshes;
	Buffer<Model> models;
	
	ivector<Mesh> cpu_meshes;
	ivector<Model> cpu_models;
	stx::hashmap<ID, uint> cpu_modelIndices; // Mapping from ID to index into models
};

// Structure storing model data as they're being loaded. After all models are
// loaded in, it can be uploaded to GPU by converting it into a ModelBuffer
struct ModelList {
	
	// Parse a model file, and append it to the list
	void addModel(std::string_view name, std::span<char const> model);
	
	// Convert into a ModelBuffer. The instance must be moved in,
	// so all CPU-side resources are freed
	auto upload(vuk::Allocator&) && -> ModelBuffer;
	
private:
	
	pvector<Material> m_materials;
	pvector<IndexType> m_indices;
	pvector<VertexType> m_vertices;
	
	ivector<Mesh> m_meshes; // Mesh descriptors, for access to index buffers
	ivector<Model> m_models; // Model descriptors, for access to m_meshes
	stx::hashmap<ID, uint> m_modelIndices; // Mapping of model IDs to their index in m_models
	
};

}
