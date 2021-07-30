#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/module/indirect.hpp"
#include "gfx/module/sky.hpp"
#include "gfx/module/ibl.hpp"
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
	
	// Prepare for rendering into managed images of specified size
	Forward(vuk::PerThreadContext&, uvec2 size);
	
	auto size() const -> uvec2 { return m_size; }
	
	// Perform Z-prepass, filling in the Depth_n image
	auto zPrepass(vuk::Buffer world, Indirect const&, MeshBuffer const&) -> vuk::RenderGraph;
	
	// Using Depth_n, render into Color_n
	auto draw(vuk::Buffer world, Indirect const&, MeshBuffer const&, Sky const&, IBLMap const&) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
	uvec2 m_size;
	
};

}
