#pragma once

#include "vuk/Allocator.hpp"
#include "util/types.hpp"
#include "gfx/resource.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"

namespace minote {

// A list of meshlet instances, created by taking the list of scene
// objects and splitting each one into its component meshlets
struct InstanceList {
	
	struct Instance {
		uint objectIdx; // Index into every ObjectBuffer buffer
		uint meshletIdx; // Index into ModelBuffer::meshlets
	};
	
	Buffer<Instance> instances;
	uint instanceCount; // Can be calculated CPU-side
	
	InstanceList(vuk::Allocator&, ModelBuffer&, ObjectBuffer&);
	
	// Build required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};
/*
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
