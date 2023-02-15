#pragma once

#include <cmath>
#include "types.hpp"

namespace minote {

// Return the number a mipmaps that a square texture of the given size would have
inline auto mipmapCount(uint size) {
	
	return uint(std::floor(std::log2(size))) + 1;
	
}

// Integer division that rounds up instead of down
template<typename T, typename U>
constexpr auto divRoundUp(T n, U div) -> T {
	
	return (n + (div - U(1))) / div;
	
}

}
