#pragma once

#include <type_traits>
#include <span>
#include "vuk/Future.hpp"
#include "util/hashmap.hpp"
#include "util/vector.hpp"
#include "util/types.hpp"
#include "util/span.hpp"
#include "util/math.hpp"
#include "util/id.hpp"
#include "tools/modelSchema.hpp"

namespace minote {

struct Material {
	enum struct Type: u32 {
		None = 0, // Invalid
		PBR = 1,
		Count
	};
	
	vec4 color;
	vec3 emissive;
	u32 id;
	f32 metalness;
	f32 roughness;
	vec2 pad0;
};

using MaterialType = Material::Type;

struct Meshlet {
	u32 materialIdx;
	
	u32 indexOffset;
	u32 indexCount;
	u32 vertexOffset;
	
	vec3 boundingSphereCenter;
	f32 boundingSphereRadius;
};

struct Model {
	u32 meshletOffset;
	u32 meshletCount;
};

// A set of buffers storing vertex data for all models, and how to access each
// model within the buffer
struct ModelBuffer {
	vuk::Future materials; // Buffer<Material>
	vuk::Future triIndices; // Buffer<u32>
	vuk::Future vertIndices; // Buffer<VertIndexType>
	vuk::Future vertices; // Buffer<VertexType>
	vuk::Future normals; // Buffer<NormalType>
	
	vuk::Future meshlets; // Buffer<Meshlet>
	vuk::Future models; // Buffer<Model>
	
	ivector<Meshlet> cpu_meshlets;
	ivector<Model> cpu_models;
	hashmap<ID, u32> cpu_modelIndices;
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
	pvector<u32> m_triIndices;
	pvector<VertIndexType> m_vertIndices;
	pvector<VertexType> m_vertices;
	pvector<NormalType> m_normals;
	
	ivector<Meshlet> m_meshlets; // Meshlet descriptors, for access to index buffers
	ivector<Model> m_models; // Model descriptors, for access to m_modelMeshes
	hashmap<ID, u32> m_modelIndices; // Mapping of model IDs to their index in m_models
	
};

}
