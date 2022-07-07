#pragma once

#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

struct BVH {
	
	static void compile(vuk::PerThreadContext&);
	
	static void debugDrawAABBs(Frame&, Texture2D target, InstanceList);
	
};

}
