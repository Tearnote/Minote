#pragma once

#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "gfx/module/meshes.hpp"

namespace minote::gfx {

// Acceleration structures for GPU compute raytracing. A triangle BVH is built
// for each model at startup, and then an instance BVH every frame. This
// two-level structure can be used for raytracing on GPU, such as light source
// visibility test.
struct Bvh {
	
	// Single node of a BVH. Contents depend on the isLeaf member.
	union Node {
		
		// Internal node
		struct Inter {
			
			vec3 aabbMin;
			u32 isLeaf;
			vec3 aabbMax;
			u32 miss;
			
		};
		
		// Leaf node, pointing at a specific triangle
		struct Leaf {
			
			uvec3 indices;
			u32 isLeaf;
			vec3 pad1;
			u32 miss;
			
		};
		
		
		Inter inter;
		Leaf leaf;
		
	};
	
	// BVH descriptor, pointing at the first node of a mesh's BVH
	// in the combined buffer
	struct Descriptor {
		
		u32 offset;
		u32 nodeCount;
		
	};
	
	static_assert(sizeof(Node::Inter) == sizeof(Node::Leaf));
	static_assert(sizeof(Node::Inter) == sizeof(Node));
	
	vuk::Unique<vuk::Buffer> bvhBuf;
	vuk::Unique<vuk::Buffer> descriptorsBuf;
	
	// Generate and upload a BVH of every mesh. Call this once, before meshes
	// are uploaded.
	void generateMeshesBvh(vuk::PerThreadContext&, Meshes const&);
	
};

}
