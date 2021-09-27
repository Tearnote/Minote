#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

struct Visibility {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(vuk::RenderGraph&, Texture2D visbuf, Texture2D depth, Buffer<World>,
		DrawableInstanceList const&, MeshBuffer const&);
	
};

}
