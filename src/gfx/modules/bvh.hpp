#pragma once

#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "gfx/modules/meshes.hpp"

namespace minote::gfx::modules {

struct Bvh {
	
	union Node {
		
		struct Inter {
			
			vec3 aabbMin;
			u32 isLeaf;
			vec3 aabbMax;
			u32 miss;
			
		};
		
		struct Leaf {
			
			uvec3 indices;
			u32 isLeaf;
			vec3 pad1;
			u32 miss;
			
		};
		
		
		Inter inter;
		Leaf leaf;
		
	};
	
	struct Descriptor {
		
		u32 offset;
		u32 nodeCount;
		
	};
	
	static_assert(sizeof(Node::Inter) == sizeof(Node::Leaf));
	static_assert(sizeof(Node::Inter) == sizeof(Node));
	
	vuk::Unique<vuk::Buffer> bvhBuf;
	vuk::Unique<vuk::Buffer> descriptorsBuf;
	
	void generateMeshesBvh(vuk::PerThreadContext&, Meshes const&);
	
};

}
