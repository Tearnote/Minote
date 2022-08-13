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
	uint triangleCount;
	
	InstanceList(vuk::Allocator&, ModelBuffer&, ObjectBuffer&);
	
	// Build required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

struct TriangleList {
	
	using Command = VkDrawIndexedIndirectCommand;
	
	Buffer<Command> command;
	Buffer<uint> indices;
	
	TriangleList(vuk::Allocator&, ModelBuffer&, InstanceList&);
	
	// Compile required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

}
