#pragma once

#include "gfx/resources/texture2d.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

struct Clear {
	
	static void apply(Frame&, Texture2D target, vuk::ClearColor);
	
};

}
