#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/cubemap.hpp"

namespace minote::gfx {

using namespace base;

// Performs filtering of a cubemap, generating increasingly blurred versions
// of each mipmap. Useful for IBL with a range of roughness values.
struct CubeFilter {
	
	// 1st mip is perfect specular, next mips are increasingly rough
	static constexpr auto MipCount = 1u + 7u;
	
	// The technique only supports cubemaps of this size
	static constexpr auto BaseSize = 256u;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Using mip0 of src as input, generate MipCount mips in dst.
	static auto apply(string_view name, Cubemap& src, Cubemap& dst) -> vuk::RenderGraph;
	
};

}
