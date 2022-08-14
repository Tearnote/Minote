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

struct Worklist {
	
	static constexpr auto TileSize = 8u;
	static constexpr auto ListCount = +Material::Type::Count;
	
	Buffer<uint4> counts; // x holds tile count, yz are 1 (for dispatch indirect)
	Buffer<uint> lists;
	uint2 tileArea; // How many tiles fit in each dimension
	
	Worklist(vuk::Allocator&, ModelBuffer&, InstanceList&, TriangleList&, Visibility&, uint2 extent);
	
	// Build required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

}
