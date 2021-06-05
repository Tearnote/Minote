#pragma once

#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

struct Indirect {
	
	struct Command {
		
		u32 indexCount;
		u32 instanceCount;
		u32 firstIndex;
		i32 vertexOffset;
		u32 firstInstance;
		// =====
		f32 meshRadius;
		
	};
	
	struct Instance {
		
		mat4 transform;
		vec4 tint;
		f32 roughness;
		f32 metalness;
		u32 meshID;
		f32 pad0;
		
	};
	
	usize commandsCount;
	vuk::Buffer commandsBuf;
	
	usize instancesCount;
	vuk::Buffer instancesBuf;
	vuk::Buffer instancesCulledBuf;
	
	Indirect(vuk::PerThreadContext&, Objects const&, Meshes const&);
	
};

}
