#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/effects/instanceList.hpp"
#include "gfx/effects/quadBuffer.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/models.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

struct PBR {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static void apply(vuk::RenderGraph&, Texture2D color, Texture2D visbuf,
		Worklist const&, Buffer<World>, ModelBuffer const&, DrawableInstanceList const&,
		Cubemap ibl, Buffer<vec3> sunLuminance, Texture3D aerialPerspective);
	
	static void applyQuad(vuk::RenderGraph&, QuadBuffer&, Worklist const&,
		Buffer<World>, ModelBuffer const&, DrawableInstanceList const&,
		Cubemap ibl, Buffer<vec3> sunLuminance, Texture3D aerialPerspective);
	
};

}
