#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

// Indirect module turns object lists into instance buffers and a command buffer
// for indirect drawing. A pass on the buffers must be performed to sort
// the instances and perform frustum culling.
struct Indirect {
	
	static constexpr auto Commands_n = "indirect_commands";
	static constexpr auto MeshIndex_n = "indirect_meshindex";
	static constexpr auto Transform_n = "indirect_transform";
	static constexpr auto Material_n = "indirect_material";
	static constexpr auto MeshIndexCulled_n = "indirect_meshindex_culled";
	static constexpr auto TransformCulled_n = "indirect_transform_culled";
	static constexpr auto MaterialCulled_n = "indirect_material_culled";
	
	usize commandsCount;
	Buffer<VkDrawIndexedIndirectCommand> commandsBuf;
	
	usize instancesCount;
	Buffer<u32> meshIndexCulledBuf;
	Buffer<vec4[3]> transformCulledBuf;
	Buffer<ObjectPool::Material> materialCulledBuf;
	
	// Upload object data into temporary buffers.
	Indirect(vuk::PerThreadContext&, vuk::Name, ObjectPool const&, MeshBuffer const&);
	
	// Clean up owned resources.
	void recycle(vuk::PerThreadContext&);
	
	// Perform sorting and frustum culling to fill in the Culled_n buffers.
	auto sortAndCull(World const&, MeshBuffer const&) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
	Buffer<u32> meshIndexBuf;
	Buffer<ObjectPool::Transform> transformBuf;
	Buffer<ObjectPool::Material> materialBuf;
	
};

}
