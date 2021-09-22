#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/effects/indirect.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/meshes.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

struct PBR {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(vuk::RenderGraph&, Texture2D color, Texture2D visbuf, Texture2D depth,
		Buffer<World>, MeshBuffer const&, Indirect const&, Sky const&, Cubemap ibl);
	
};

}