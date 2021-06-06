#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/world.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

struct Indirect {
	
	static constexpr auto Commands_n = "indirect_commands";
	static constexpr auto Instances_n = "indirect_instances";
	static constexpr auto InstancesCulled_n = "indirect_instances_culled";
	
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
	
	auto frustumCull(World const&) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
};

}
