#pragma once

#include "vuk/Allocator.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/resource.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "util/types.hpp"
#include "util/math.hpp"

namespace minote {

struct Visibility {
	
	Texture2D<uint> visibility;
	Texture2D<float> depth;
	
	Visibility(vuk::Allocator&, ModelBuffer&, ObjectBuffer&,
		InstanceList&, TriangleList&, uint2 extent, float4x4 viewProjection);
	
	// Build required shaders; potional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};
/*
struct Worklist {
	
	static constexpr auto TileSize = float2{8, 8};
	static constexpr auto ListCount = +MaterialType::Count;
	
	Buffer<uint4> counts; // x holds tile count, yz are 1 (for dispatch indirect)
	Buffer<uint> lists;
	uint2 tileDimensions; // How many tiles fit in each dimension
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static auto create(Pool&, Frame&, vuk::Name, Texture2D visbuf, TriangleList) -> Worklist;
	
};
*/
}
