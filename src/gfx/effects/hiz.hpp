#pragma once

#include "vuk/Context.hpp"
#include "vuk/Name.hpp"
#include "gfx/resources/texture2dms.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

struct HiZ {
	
	static void compile(vuk::PerThreadContext&);
	
	static auto make(Pool&, vuk::Name, Texture2DMS depth) -> Texture2D;
	
	static void fill(Frame&, Texture2D hiz, Texture2DMS depth);
	
};

}
