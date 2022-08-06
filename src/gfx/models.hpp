#pragma once

#include <type_traits>
#include <span>
#include "vuk/Future.hpp"
#include "gfx/resource.hpp"
#include "util/hashmap.hpp"
#include "util/vector.hpp"
#include "util/types.hpp"
#include "util/span.hpp"
#include "util/math.hpp"
#include "util/id.hpp"
#include "tools/modelSchema.hpp"

namespace minote {

struct Material {
	enum struct Type: uint {
		None = 0, // Invalid
		PBR = 1,
		Count
	};
	
	float4 color;
	float3 emissive;
	uint id;
	float metalness;
	float roughness;
	float2 pad0;
};

using MaterialType = Material::Type;

struct Meshlet {
	uint materialIdx;
	
	uint indexOffset;
	uint indexCount;
	uint vertexOffset;
	
	float3 boundingSphereCenter;
	float boundingSphereRadius;
};

struct Model {
	uint meshletOffset;
	uint meshletCount;
};

// A set of buffers storing vertex data for all models, and how to access each
// model within the buffer
struct ModelBuffer {
	Buffer<Material> materials;
	Buffer<uint> triIndices;
	Buffer<VertIndexType> vertIndices;
	Buffer<VertexType> vertices;
	Buffer<NormalType> normals;
	
	Buffer<Meshlet> meshlets;
	Buffer<Model> models;
	
	ivector<Meshlet> cpu_meshlets;
	ivector<Model> cpu_models;
	hashmap<ID, uint> cpu_modelIndices;
};

// Structure storing model data as they're being loaded. After all models are
// loaded in, it can be uploaded to GPU by converting it into a ModelBuffer
struct ModelList {
	
	// Parse a model file, and append it to the list
	void addModel(string_view name, span<char const> model);
	
	// Convert into a ModelBuffer. The instance must be moved in,
	// so all CPU-side resources are freed
	auto upload(vuk::Allocator&) && -> ModelBuffer;
	
private:
	
	pvector<Material> m_materials;
	pvector<uint> m_triIndices;
	pvector<VertIndexType> m_vertIndices;
	pvector<VertexType> m_vertices;
	pvector<NormalType> m_normals;
	
	ivector<Meshlet> m_meshlets; // Meshlet descriptors, for access to index buffers
	ivector<Model> m_models; // Model descriptors, for access to m_modelMeshes
	hashmap<ID, uint> m_modelIndices; // Mapping of model IDs to their index in m_models
	
};

}
