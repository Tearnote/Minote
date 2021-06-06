#pragma once

#include <cmath>
#include "volk.h"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

constexpr auto mipmapCount(u32 size) {
	return u32(std::floor(std::log2(size))) + 1;
}

}
