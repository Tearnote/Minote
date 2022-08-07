#pragma once

#include "vuk/Allocator.hpp"
#include "util/array.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/resource.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"

namespace minote {
/*
// A GPU-side list of meshlet instances, created by taking the list of scene
// objects and splitting each one into its component meshlets
struct InstanceList {
	
	using Transform = array<float4, 3>;
	
	struct Instance {
		uint objectIdx; // Index into colors, transforms, prevTransforms
		uint meshletIdx; // Index into ModelBuffer::meshlets
	};
	
	Buffer<float4> colors;
	Buffer<Transform> transforms;
	Buffer<Transform> prevTransforms;
	
	Buffer<Instance> instances;
	
	uint triangleCount;
	
	InstanceList(vuk::Allocator&, ModelBuffer&, ObjectPool const&);
	
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
	
	static auto fromInstances(InstanceList, Pool&, Frame&, vuk::Name,
		Texture2D hiZ, uint2 hiZInnerSize, float4x4 view, float4x4 projection) -> TriangleList;
	
	// Compile required shaders; optional
	static void compile();
	
};
*/
}
