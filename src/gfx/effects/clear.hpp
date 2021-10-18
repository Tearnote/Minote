#pragma once

#include "vuk/RenderGraph.hpp"
#include "gfx/resources/texture2d.hpp"

namespace minote::gfx {

using namespace base;

struct Clear {
	
	static void apply(vuk::RenderGraph&, Texture2D target, vuk::ClearColor);
	
};

}
