#pragma once

#include "vuk/Context.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/resources/texture2dms.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/models.hpp"
#include "gfx/frame.hpp"
#include "gfx/util.hpp"

namespace minote {

struct Visibility {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(Frame&, Texture2DMS visbuf, Texture2DMS depth, TriangleList);
	
};

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

}
