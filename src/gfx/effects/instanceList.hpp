#pragma once

#include "util/array.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

struct InstanceList {
	
	using Transform = array<vec4, 3>;
	
	struct Instance {
		u32 objectIdx;
		u32 meshletIdx;
	};
	
	Buffer<vec4> colors;
	Buffer<Transform> transforms;
	Buffer<Transform> prevTransforms;
	
	Buffer<Instance> instances;
	
	u32 triangleCount;
	
	static auto upload(Pool&, Frame&, vuk::Name, ObjectPool const&) -> InstanceList;
	
	auto size() const -> usize { return instances.length(); }
	
};

struct TriangleList {
	
	using Command = VkDrawIndexedIndirectCommand;
	using Transform = InstanceList::Transform;
	using Instance = InstanceList::Instance;
	
	Buffer<vec4> colors;
	Buffer<Transform> transforms;
	Buffer<Transform> prevTransforms;
	
	Buffer<Instance> instances;
	Buffer<uvec4> instanceCount;
	
	Buffer<Command> command;
	Buffer<u32> indices;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromInstances(InstanceList, Pool&, Frame&, vuk::Name,
		Texture2D hiZ, uvec2 hiZInnerSize, mat4 view, mat4 projection) -> TriangleList;
	
};

}
