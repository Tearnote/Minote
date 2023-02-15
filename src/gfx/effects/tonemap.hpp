#pragma once

#include <string_view>
#include <array>
#include "vuk/Future.hpp"
#include "gfx/resource.hpp"
#include "util/types.hpp"
#include "util/math.hpp"

namespace minote {

// A stateful tonemapper, adaptable to HDR displays and various viewing conditions
// Based on work of Timothy Lottes, https://www.shadertoy.com/view/XljBRK
struct Tonemap {
	
	float contrast = 1.4f;
	float shoulder = 1.0f;
	float hdrMax = 64.0f;
	float midIn = 0.18f;
	float midOut = 0.18f;
	float3 saturation = {0.0f, 0.0f, 0.0f};
	float3 crosstalk = {64.0f, 32.0f, 128.0f};
	float3 crosstalkSaturation = {4.0f, 1.0f, 16.0f};
	
	// Tonemap and gamma-correct the HDR input into a new SDR output texture
	auto apply(Texture2D<float4> source) -> Texture2D<float4>;
	
	// Build required shaders; optional
	static void compile();
	
	// Draw debug controls for this instance
	void drawImguiDebug(std::string_view name);
	
private:
	
	static inline bool m_compiled = false;
	
	auto genConstants() -> std::array<float4, 4>;
	
};

}
