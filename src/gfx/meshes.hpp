#pragma once

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

// A set of buffers storing vertex data for all meshes, and how to access each
// mesh within the buffer.
struct MeshBuffer {
	
	// These three are indexed together
	vuk::Unique<vuk::Buffer> verticesBuf;
	vuk::Unique<vuk::Buffer> normalsBuf;
	vuk::Unique<vuk::Buffer> colorsBuf;
	
	vuk::Unique<vuk::Buffer> indicesBuf;
	
	// Describes offsets of each mesh. Also contains a few per-mesh properties,
	// which can be useful for constructing spatial structures.
	struct Descriptor {
		
		u32 indexOffset;
		u32 indexCount;
		u32 vertexOffset;
		f32 radius;
		vec3 aabbMin;
		vec3 aabbMax;
		
	};
	svector<Descriptor, 256> descriptors; // Packed list of descriptors
	hashmap<ID, usize> descriptorIDs; // Mapping from IDs to descriptor indices
	
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
	
	using Descriptor = MeshBuffer::Descriptor;
	
	svector<Descriptor, 256> descriptors;
	hashmap<ID, usize> descriptorIDs;
	
	std::vector<vec3> vertices;
	std::vector<vec3> normals;
	std::vector<u16vec4> colors;
	std::vector<u16> indices;
	
};

}
