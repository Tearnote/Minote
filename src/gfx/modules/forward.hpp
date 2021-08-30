#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/modules/indirect.hpp"
#include "gfx/modules/sky.hpp"
#include "gfx/world.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Forward PBR renderer of mesh instances. Uses Z-prepass.
// Uses one light source, one diffuse+specular cubemap, and draws a skyline
// in the background.
struct Forward {
	
	static constexpr auto Depth_n = "forward_depth";
	static constexpr auto Color_n = "forward_color";
	
	static constexpr auto ColorFormat = vuk::Format::eR16G16B16A16Sfloat;
	static constexpr auto DepthFormat = vuk::Format::eD32Sfloat;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Prepare for rendering into managed images of specified size
	Forward(vuk::PerThreadContext&, uvec2 size);
	
	auto size() const -> uvec2 { return m_size; }
	
	// Perform Z-prepass, filling in the Depth_n image
	auto zPrepass(Buffer<World>, Indirect const&, MeshBuffer const&) -> vuk::RenderGraph;
	
	// Using Depth_n, render into Color_n
	auto draw(Buffer<World>, Indirect const&, MeshBuffer const&, Sky const&, Cubemap const&) -> vuk::RenderGraph;
	
private:
	
	uvec2 m_size;
	
};

}
