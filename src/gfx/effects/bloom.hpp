#pragma once

#include "vuk/Future.hpp"
#include "util/string.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/resource.hpp"

namespace minote {

// Blend an image with a blurred version of itself.
// For use on HDR source
struct Bloom {
	
	static constexpr auto Format = vuk::Format::eB10G11R11UfloatPack32;
	
	// Apply bloom to the specified image
	auto apply(Texture2D<float4> target) -> Texture2D<float4>;
	
	// Build required shaders; optional
	static void compile();
	
	// Draw debug controls for this instance
	void drawImguiDebug(string_view name);
	
	uint passes = 6u; // More passes increases blur width
	float strength = 1.0f / 64.0f; // Output multiplier
	
private:
	
	static inline bool m_compiled = false;
	
};

}
