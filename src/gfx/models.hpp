#pragma once

#include <type_traits>
#include <span>
#include "vuk/Context.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "base/containers/hashmap.hpp"
#include "base/containers/string.hpp"
#include "base/containers/vector.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"
#include "tools/modelSchema.hpp"

namespace minote::gfx {

using namespace base;

struct Material {
	
	enum struct Type: u32 {
		None = 0, // Background - typically used for sky
		PBR = 1,
		Count
	};
	
	vec4 color;
	u32 id;
	f32 metalness;
	f32 roughness;
	u32 pad0;
	
};

using MaterialType = Material::Type;

struct Mesh {
	mat4 transform;
	u32 indexOffset;
	u32 indexCount;
	u32 materialIdx;
	f32 radius;
};

// A set of buffers storing vertex data for all models, and how to access each
// model within the buffer.
struct ModelBuffer {
	
	Buffer<Material> materials;
	Buffer<tools::IndexType> indices;
	Buffer<tools::VertexType> vertices;
	Buffer<tools::NormalType> normals;
	
	Buffer<Mesh> meshes;
	
	hashmap<ID, ivector<u32, 16>> cpu_modelMeshes;
	ivector<Mesh> cpu_meshes;
	
};

// Structure storing model data as they're being loaded. After all models are
// loaded in, it can be uploaded to GPU by converting it into a ModelBuffer.
struct ModelList {
	
	// Parse a model file, and append it to the list.
	void addModel(string_view name, std::span<char const> model);
	
	// Convert into a ModelBuffer. The instance must be moved in,
	// so all CPU-side resources are freed.
	auto upload(Pool&, vuk::Name) && -> ModelBuffer;
	
private:
	
	pvector<Material> m_materials;
	pvector<tools::IndexType> m_indices;
	pvector<tools::VertexType> m_vertices;
	pvector<tools::NormalType> m_normals;
	
	// Mapping from model ID to a list of mesh indices
	hashmap<ID, ivector<u32, 16>> m_modelMeshes;
	ivector<Mesh> m_meshes;
	
	
};

}