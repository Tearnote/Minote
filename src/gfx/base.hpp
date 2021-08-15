#pragma once

#include "vuk/Types.hpp"
#include "base/concepts.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Return the number a mipmaps that a square texture of the given size would have.
constexpr auto mipmapCount(u32 size) {
	return u32(floor(log2(size))) + 1;
}

// Conversion from vec[n] to vuk::Extent[n]D

template<arithmetic T>
constexpr auto vukExtent(vec<2, T> v) -> vuk::Extent2D { return {v[0], v[1]}; }

template<arithmetic T>
constexpr auto vukExtent(vec<3, T> v) -> vuk::Extent3D { return {v[0], v[1], v[2]}; }

}
