#pragma once

#include "util/types.hpp"
#include "util/math.hpp"

namespace minote {

// Return the number a mipmaps that a square texture of the given size would have
constexpr auto mipmapCount(uint size) {
	
	return uint(floor(log2(size))) + 1;
	
}

}
