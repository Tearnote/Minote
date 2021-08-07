#pragma once

#include <type_traits>
#include <span>
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "base/container/hashmap.hpp"
#include "base/container/string.hpp"
#include "base/container/vector.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"

namespace minote::gfx {

using namespace base;

// Mesh metadata structure, for vertex access and analysis
struct MeshDescriptor {
	
	u32 indexOffset;
	u32 indexCount;
	u32 vertexOffset;
	f32 radius;
	
};

// Ensure fast operation in large containers
static_assert(std::is_trivially_constructible_v<MeshDescriptor>);

// A set of buffers storing vertex data for all meshes, and how to access each
// mesh within the buffer.
struct MeshBuffer {
	
	// These three are indexed together
	vuk::Unique<vuk::Buffer> verticesBuf;
	vuk::Unique<vuk::Buffer> normalsBuf;
	vuk::Unique<vuk::Buffer> colorsBuf;
	
	vuk::Unique<vuk::Buffer> indicesBuf;
	
	ivector<MeshDescriptor> descriptors; // CPU-side mesh metadata
	vuk::Unique<vuk::Buffer> descriptorBuf; // GPU-accessible mesh metadata
	hashmap<ID, usize> descriptorIDs; // Mapping from IDs to descriptor buffer indices
	
};

// Structure storing mesh data as they're being loaded. After all meshes are
// loaded in, it can be uploaded to GPU by converting it into a MeshBuffer.
struct MeshList {
	
	// Parse a GLTF mesh, and append it to the list. Binary format is expected
	// (.glb), and very specific contents:
	// 1 mesh, with 1 primitive,
	// indexed with u16 scalars,
	// with following vertex attributes:
	// - POSITION (f32 vec3),
	// - NORMAL (f32 vec3),
	// - COLOR_0 (u16 vec4)
	void addGltf(string_view name, std::span<char const> mesh);
	
	// Convert into a MeshBuffer. The instance must be moved in,
	// so all CPU-side resources are freed.
	auto upload(vuk::PerThreadContext&) && -> MeshBuffer;
	
private:
	
	ivector<MeshDescriptor> descriptors;
	hashmap<ID, usize> descriptorIDs;
	
	ivector<vec3> vertices;
	ivector<vec3> normals;
	ivector<u16vec4> colors;
	ivector<u16> indices;
	
};

}
