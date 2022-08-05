#pragma once

#include "vuk/Future.hpp"
#include "util/string.hpp"
#include "util/array.hpp"
#include "util/types.hpp"
#include "util/math.hpp"

namespace minote {

// A stateful tonemapper, adaptable to HDR displays and various viewing conditions
// Based on work of Timothy Lottes, https://www.shadertoy.com/view/XljBRK
struct Tonemap {
	
	f32 contrast = 1.4f;
	f32 shoulder = 1.0f;
	f32 hdrMax = 64.0f;
	f32 midIn = 0.18f;
	f32 midOut = 0.18f;
	vec3 saturation = vec3(0.0f);
	vec3 crosstalk = vec3{64.0f, 32.0f, 128.0f};
	vec3 crosstalkSaturation = vec3{4.0f, 1.0f, 16.0f};
	
	// Tonemap and gamma-correct the HDR input into a new SDR output texture
	auto apply(vuk::Future source) -> vuk::Future;
	
	// Build required shaders; optional
	static void compile();
	
	// Draw debug controls for this instance
	void drawImguiDebug(string_view name);
	
private:
	
	static inline bool m_compiled = false;
	
	auto genConstants() -> array<vec4, 4>;
	
};

}
