#pragma once

#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/effects/quadBuffer.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

struct Shadow {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void genBuffer(Frame&, Texture2DMS shadowbuf, InstanceList);
	
	static void genShadow(Frame&, Texture2DMS shadowbuf, Texture2D shadowOut, QuadBuffer&, InstanceList);
	
};

}
