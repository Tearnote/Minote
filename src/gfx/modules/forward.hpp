#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/modules/indirect.hpp"
#include "gfx/modules/sky.hpp"
#include "gfx/modules/ibl.hpp"

namespace minote::gfx::modules {

struct Forward {
	
	static constexpr auto Depth_n = "forward_depth";
	static constexpr auto Color_n = "forward_color";
	static constexpr auto Resolved_n = "forward_resolved";
	
	static constexpr auto ColorFormat = vuk::Format::eR16G16B16A16Sfloat;
	static constexpr auto DepthFormat = vuk::Format::eD32Sfloat;
	static constexpr auto SampleCount = vuk::Samples::e4;
	
	vuk::Extent2D size;
	
	Forward(vuk::PerThreadContext&, vuk::Extent2D targetSize);
	
	auto zPrepass(vuk::Buffer world, Indirect&, Meshes&) -> vuk::RenderGraph;
	
	auto draw(vuk::Buffer world, Indirect&, Meshes&, Sky&, IBLMap&) -> vuk::RenderGraph;
	
	auto resolve() -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
};

}
