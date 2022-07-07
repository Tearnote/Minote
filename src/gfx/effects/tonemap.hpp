#pragma once

#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

struct Tonemap {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Perform tonemaping from source to target. The target image is not created.
	static void apply(Frame&, Texture2D source, Texture2D target);
	
};

}
