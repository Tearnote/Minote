#pragma once

#include "vuk/Types.hpp"
#include "gfx/resource.hpp"

namespace minote {

struct HiZ {
	
	constexpr static auto HiZFormat = vuk::Format::eR32Sfloat;
	
	// Depth pyramid; always a POT square
	Texture2D<float> hiz;
	
	// Generate a HiZ image from a depth buffer
	HiZ(Texture2D<float> depth);
	
	// Compile required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

}
