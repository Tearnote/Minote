#pragma once

#include "vuk/Context.hpp"
#include "gfx/resources/texture3d.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/effects/quadBuffer.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/frame.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

struct PBR {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(Frame&, QuadBuffer&, Worklist, DrawableInstanceList,
		Cubemap ibl, Buffer<vec3> sunLuminance, Texture3D aerialPerspective);
	
};

}
