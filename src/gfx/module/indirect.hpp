#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "gfx/module/world.hpp"
#include "gfx/meshes.hpp"
#include "gfx/objects.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

// Indirect module turns object lists into instance buffers and a command buffer
// for indirect drawing. A pass on the buffers must be performed to sort
// the instances and perform frustum culling.
struct Indirect {
	
	static constexpr auto Commands_n = "indirect_commands";
	static constexpr auto Metadata_n = "indirect_metadata";
	static constexpr auto MeshIndex_n = "indirect_meshindex";
	static constexpr auto Transform_n = "indirect_transform";
	static constexpr auto Material_n = "indirect_material";
	static constexpr auto TransformCulled_n = "indirect_transform_culled";
	static constexpr auto MaterialCulled_n = "indirect_material_culled";
	
	// A VkDrawIndexedIndirectCommand struct, expanded with custom data
	struct Command {
		
		u32 indexCount;
		u32 instanceCount;
		u32 firstIndex;
		i32 vertexOffset;
		u32 firstInstance;
		// =====
		f32 meshRadius;
		
	};
	
	usize commandsCount;
	vuk::Buffer commandsBuf;
	
	usize instancesCount;
	vuk::Buffer transformCulledBuf;
	vuk::Buffer materialCulledBuf;
	
	// Upload object data into temporary buffers.
	Indirect(vuk::PerThreadContext&, Objects const&, Meshes const&);
	
	// Perform sorting and frustum culling to fill in the Culled_n buffers.
	auto sortAndCull(World const&) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
	vuk::Buffer metadataBuf;
	vuk::Buffer meshIndexBuf;
	vuk::Buffer transformBuf;
	vuk::Buffer materialBuf;
	
};

}
