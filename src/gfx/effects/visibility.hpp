#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/texture2dms.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/world.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct Visibility {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(vuk::RenderGraph&, Texture2D visbuf, Texture2D depth, Buffer<World>,
		DrawableInstanceList const&, ModelBuffer const&);
	
	static void applyMS(vuk::RenderGraph&, Texture2DMS visbuf, Texture2DMS depth, Buffer<World>,
		DrawableInstanceList const&, ModelBuffer const&);
	
};

struct Worklist {
	
	static constexpr auto TileSize = vec2{8, 8};
	static constexpr auto ListCount = +MaterialType::Count;
	
	Buffer<uvec4> counts; // x holds tile count, yz are 1 (for dispatch indirect)
	Buffer<u32> lists;
	uvec2 tileDimensions; // How many tiles fit in each dimension
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static auto create(Pool&, vuk::RenderGraph&, vuk::Name, Texture2D visbuf,
		DrawableInstanceList const&, ModelBuffer const&) -> Worklist;
	
};

}
