#pragma once

#include "vuk/Future.hpp"
#include "util/types.hpp"

namespace minote {

// Bloom effect. Blends an image with a blurred version of itself.
// This implementation has no thresholding to better mimic naked-eye glare,
// and uses a low-pass filter to avoid fireflies that are common in HDR source
// images. Relative blur width is resolution-independent
struct Bloom {
	
	static constexpr auto Format = vuk::Format::eB10G11R11UfloatPack32;
	
	// Apply bloom to the specified image
	auto apply(vuk::Future target) -> vuk::Future;
	
	// Build required shaders; optional
	static void compile();
	
	u32 passes = 6u; // More passes increases blur width
	f32 strength = 0.0f / 64.0f; // Output multiplier
	
private:
	
	static inline bool m_compiled = false;
	
};

}
