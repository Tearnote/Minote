#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/objects.hpp"
#include "gfx/engine.hpp"
#include "gfx/models.hpp"
#include "gfx/world.hpp"
#include "util/types.hpp"
#include "util/math.hpp"

namespace minote {

struct Frame {
	
	explicit Frame(Engine&, vuk::RenderGraph&);
	void draw(Texture2D target, ObjectPool&, bool flush);
	
	vuk::PerThreadContext& ptc;
	vuk::RenderGraph& rg;
	Pool& framePool;
	Pool& swapchainPool;
	Pool& permPool;
	ModelBuffer& models;
	World& cpu_world;
	Buffer<World> world;
	
};

}
