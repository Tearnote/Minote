#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

using namespace base;

// Bloom effect. Blends an image with a blurred version of itself.
// This implementation has no thresholding to better mimic naked-eye glare,
// and uses a low-pass filter to avoid fireflies that are common in HDR source
// images. Relative blur width is resolution-independent.
struct Bloom {
	
	//TODO modify shaders to make the more compact format work
	// static constexpr auto BloomFormat = vuk::Format::eB10G11R11UfloatPack32;
	static constexpr auto BloomFormat = vuk::Format::eR16G16B16A16Sfloat;
	// More passes increases blur width at a small performance cost
	static constexpr auto BloomPasses = 6u;
	// Because the blending is additive, the strength multiplier needs to be very small
	static constexpr auto BloomStrength = 1.0f / 64.0f;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	// Create a pass that applies bloom to the specified image.
	static void apply(vuk::RenderGraph&, Pool&, Texture2D target);
	
};

}
