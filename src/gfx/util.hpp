#pragma once

#include "vuk/CommandBuffer.hpp"
#include "vuk/Types.hpp"
#include "vuk/Name.hpp"
#include "util/concepts.hpp"
#include "util/string.hpp"
#include "util/types.hpp"
#include "util/span.hpp"
#include "util/math.hpp"

namespace minote {

// Return the number a mipmaps that a square texture of the given size would have
constexpr auto mipmapCount(uint size) {
	
	return uint(floor(log2(size))) + 1;
	
}

// Rounded up division
template<usize N, integral T>
constexpr auto divRoundUp(vec<N, T> _v, vec<N, T> _div) -> vec<N, T> {
	
	return (_v - T(1)) / _div + T(1);
	
}

template<usize N, integral T>
constexpr auto divRoundUp(vec<N, T> _v, T _div) -> vec<N, T> {
	
	return (_v - vec<N, T>(1)) / _div + vec<N, T>(1);
	
}

template<integral T>
constexpr auto divRoundUp(T _val, T _div) -> T {
	
	return (_val - T(1)) / _div + T(1);
	
}

// Pack two values in a uint. x is in lower bits, y in upper
constexpr auto uintFromuint16(uint2 _v) -> uint {
	
	return (_v.y() << 16) | _v.x();
	
}

}
