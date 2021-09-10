#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/effects/indirect.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

// Forward PBR renderer of mesh instances. Uses Z-prepass.
// Uses one light source, one diffuse+specular cubemap, and draws a skyline
// in the background.
struct Forward {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Perform Z-prepass, filling in the depth texture
	static void zPrepass(vuk::RenderGraph&, Texture2D depth, Buffer<World>,
		Indirect const&, MeshBuffer const&);
	
	// Perform shading on color image, making use of depth data
	static void draw(vuk::RenderGraph&, Texture2D color, Texture2D depth, Buffer<World>,
		Indirect const&, MeshBuffer const&, Sky const&, Cubemap ibl);
	
};

}
