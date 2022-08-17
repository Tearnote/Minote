#pragma once

#include "vuk/Types.hpp"
#include "gfx/resource.hpp"

namespace minote {

struct HiZ {
	
	constexpr static auto HiZFormat = vuk::Format::eR32Sfloat;
	
	// Generate a HiZ image from a depth buffer
	static auto create(Texture2D<float> depth) -> Texture2D<float>;
	
	// Compile required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

}
