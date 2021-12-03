#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2dms.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/models.hpp"
#include "gfx/world.hpp"

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
	
	Texture2D velocity;
	
	vuk::Name name;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto create(vuk::RenderGraph&, Pool&,
		vuk::Name, uvec2 size, bool flushTemporal = false) -> QuadBuffer;
	
	static void clusterize(vuk::RenderGraph&, QuadBuffer&, Texture2DMS visbuf, Buffer<World>);
	
	static void genBuffers(vuk::RenderGraph&, QuadBuffer&, ModelBuffer const&,
		DrawableInstanceList const&, Buffer<World>);
	
	static void resolve(vuk::RenderGraph&, QuadBuffer&, Texture2D output, Buffer<World>);
	
};

}
