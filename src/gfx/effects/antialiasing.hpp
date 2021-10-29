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
	
	static void quadScatter(vuk::RenderGraph&, Texture2DMS visbuf, Texture2D quadbuf, Buffer<World>);
	
	static void quadResolve(vuk::RenderGraph&, Texture2D target, Texture2D quadbuf,
		Texture2D outputs, Texture2D targetHistory, Buffer<World>);
	
};

}
