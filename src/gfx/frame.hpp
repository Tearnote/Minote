#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/objects.hpp"
#include "gfx/engine.hpp"
#include "gfx/models.hpp"
#include "gfx/world.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

struct Frame {
	
	enum struct AntialiasingType {
		None = 0,
		Quad = 1,
	};
	
	explicit Frame(Engine&, vuk::RenderGraph&);
	void draw(Texture2D target, ObjectPool&, AntialiasingType, bool flush);
	
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
