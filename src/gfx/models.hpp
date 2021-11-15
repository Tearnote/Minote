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

namespace minote::gfx {

using namespace base;

// Model metadata structure, for vertex access and analysis
struct ModelDescriptor {
	
	u32 indexOffset;
	u32 indexCount;
	u32 vertexOffset;
	f32 radius;
	
};

// Ensure fast operation in large containers
static_assert(std::is_trivially_constructible_v<ModelDescriptor>);

// A set of buffers storing vertex data for all models, and how to access each
// model within the buffer.
struct ModelBuffer {
	
	// These three are indexed together
	Buffer<vec3> vertices;
	Buffer<vec3> normals;
	Buffer<u16vec4> colors;
	
	Buffer<u32> indices;
	
	Buffer<ModelDescriptor> descriptors; // GPU-accessible model metadata
	
	ivector<ModelDescriptor> cpu_descriptors; // CPU-side model metadata
	hashmap<ID, usize> cpu_descriptorIDs; // Mapping from IDs to descriptor buffer indices
	
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
	
	ivector<ModelDescriptor> m_descriptors;
	hashmap<ID, usize> m_descriptorIDs;
	
	pvector<vec3> m_vertices;
	pvector<vec3> m_normals;
	pvector<u16vec4> m_colors;
	pvector<u32> m_indices;
	
};

}
