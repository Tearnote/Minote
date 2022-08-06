#pragma once

#include "util/array.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/frame.hpp"

namespace minote {

struct InstanceList {
	
	using Transform = array<float4, 3>;
	
	struct Instance {
		uint objectIdx;
		uint meshletIdx;
	};
	
	Buffer<float4> colors;
	Buffer<Transform> transforms;
	Buffer<Transform> prevTransforms;
	
	Buffer<Instance> instances;
	
	uint triangleCount;
	
	static auto upload(Pool&, Frame&, vuk::Name, ObjectPool const&) -> InstanceList;
	
	auto size() const -> usize { return instances.length(); }
	
};

struct TriangleList {
	
	using Command = VkDrawIndexedIndirectCommand;
	using Transform = InstanceList::Transform;
	using Instance = InstanceList::Instance;
	
	Buffer<float4> colors;
	Buffer<Transform> transforms;
	Buffer<Transform> prevTransforms;
	
	Buffer<Instance> instances;
	Buffer<uint4> instanceCount;
	
	Buffer<Command> command;
	Buffer<uint> indices;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromInstances(InstanceList, Pool&, Frame&, vuk::Name,
		Texture2D hiZ, uint2 hiZInnerSize, float4x4 view, float4x4 projection) -> TriangleList;
	
};

}
