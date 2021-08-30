#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Simple post-processing effects. Currently contains tonemapping.
struct Tonemap {
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Perform tonemaping from source to target. The target image is not created.
	static auto apply(vuk::Name source, vuk::Name target, uvec2 targetSize) -> vuk::RenderGraph;
	
};

}
