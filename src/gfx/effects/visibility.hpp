#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/objects.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

struct Visibility {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(vuk::RenderGraph&, Texture2D visbuf, Texture2D depth, Buffer<World>,
		DrawableInstanceList const&, MeshBuffer const&);
	
};

struct Worklist {
	
	static constexpr auto TileSize = vec2{8, 8};
	static constexpr auto ListCount = MaterialCount;
	
	Buffer<uvec4> counts; // xyz holds group count for dispatch indirect, w is the item count
	Buffer<u32> lists;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static auto create(Pool&, vuk::RenderGraph&, vuk::Name, Texture2D visbuf,
		DrawableInstanceList const&) -> Worklist;
	
};

}
