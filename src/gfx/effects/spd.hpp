#pragma once

#include "util/types.hpp"
#include "gfx/resource.hpp"

namespace minote {

struct SPD {
	
	enum struct ReductionType: uint {
		Avg = 0,
		Min = 1,
		Max = 2,
	};
	
	// Generate up to 12 mips of the input image from its mip0
	static auto apply(Texture2D<float>, ReductionType) -> Texture2D<float>;
	
	// Compile required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

}
