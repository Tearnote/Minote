#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2dms.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

struct Antialiasing {
	
	static void compile(vuk::PerThreadContext&);
	
	static void resolveQuad(vuk::RenderGraph&, Texture2DMS visbuf, Texture2D resolved, Buffer<World>);
	
};

}
