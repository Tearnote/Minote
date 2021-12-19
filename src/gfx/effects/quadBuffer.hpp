#pragma once

#include "vuk/Context.hpp"
#include "gfx/resources/texture2dms.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

using namespace base;

struct QuadBuffer {
	
	Texture2D clusterDef;
	Texture2D jitterMap;
	Texture2D clusterOut;
	Texture2D output;
	
	Texture2D clusterDefPrev;
	Texture2D jitterMapPrev;
	Texture2D clusterOutPrev;
	Texture2D outputPrev;
	
	Texture2D normal;
	Texture2D velocity;
	
	vuk::Name name;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto create(Pool&, Frame&, vuk::Name, uvec2 size, bool flushTemporal = false) -> QuadBuffer;
	
	static void clusterize(Frame&, QuadBuffer&, Texture2DMS visbuf);
	
	static void genBuffers(Frame&, QuadBuffer&, DrawableInstanceList);
	
	static void resolve(Frame&, QuadBuffer&, Texture2D output);
	
};

}
