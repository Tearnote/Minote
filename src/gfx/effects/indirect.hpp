#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

// Indirect effect turns object lists into instance buffers and a command buffer
// for indirect drawing. A pass on the buffers must be performed to sort
// the instances and perform frustum culling.
struct Indirect {
	
	usize commandsCount;
	Buffer<VkDrawIndexedIndirectCommand> commandsBuf;
	
	usize instancesCount;
	Buffer<u32> meshIndicesCulledBuf;
	Buffer<vec4[3]> transformsCulledBuf;
	Buffer<ObjectPool::Material> materialsCulledBuf;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Upload object data into temporary buffers.
	Indirect(Pool&, vuk::Name, ObjectPool const&, MeshBuffer const&);
	
	// Perform sorting and frustum culling to fill in the Culled_n buffers.
	auto sortAndCull(World const&, MeshBuffer const&) -> vuk::RenderGraph;
	
private:
	
	Buffer<u32> meshIndicesBuf;
	Buffer<ObjectPool::Transform> transformsBuf;
	Buffer<ObjectPool::Material> materialsBuf;
	
};

}
