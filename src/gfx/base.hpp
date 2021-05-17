#pragma once

#include <cmath>
#include "volk.h"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

static constexpr auto ColorFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
static constexpr auto DepthFormat = VK_FORMAT_D16_UNORM;
static constexpr auto VerticalFov = 45_deg;
static constexpr auto NearPlane = 1_m;

constexpr auto mipmapCount(u32 size) {
	return u32(std::floor(std::log2(size))) + 1;
}

}
