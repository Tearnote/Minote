#pragma once

#include <cmath>
#include "volk.h"
#include "vuk/Types.hpp"
#include "base/concepts.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

constexpr auto mipmapCount(u32 size) {
	return u32(std::floor(std::log2(size))) + 1;
}

template<arithmetic T>
constexpr auto vukExtent(vec<2, T> v) -> vuk::Extent2D { return {v[0], v[1]}; }

template<arithmetic T>
constexpr auto vukExtent(vec<3, T> v) -> vuk::Extent3D { return {v[0], v[1], v[2]}; }

}
