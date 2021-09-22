#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

// Indirect effect turns object lists into instance buffers and a command buffer
// for indirect drawing. A pass on the buffers must be performed to sort
// the instances and perform frustum culling.
struct Indirect {
	
	using Command = VkDrawIndexedIndirectCommand;
	using MeshIndex = u32;
	using BasicTransform = ObjectPool::Transform;
	using Transform = array<vec4, 3>;
	using Material = ObjectPool::Material;
	
	usize commandsCount;
	Buffer<Command> commandsBuf;
	
	usize instancesCount;
	Buffer<MeshIndex> meshIndicesCulledBuf;
	Buffer<Transform> transformsCulledBuf;
	Buffer<Material> materialsCulledBuf;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Upload object data into temporary buffers.
	Indirect(Pool&, vuk::Name, ObjectPool const&, MeshBuffer const&);
	
	// Perform sorting and frustum culling to fill in the Culled_n buffers.
	void sortAndCull(vuk::RenderGraph&, World const&, MeshBuffer const&);
	
private:
	
	Buffer<MeshIndex> meshIndicesBuf;
	Buffer<BasicTransform> transformsBuf;
	Buffer<Material> materialsBuf;
	
};

}
