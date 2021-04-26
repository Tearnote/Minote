#pragma once

#include <cmath>
#include "glm/trigonometric.hpp"
#include "volk.h"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

static constexpr auto ColorFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
static constexpr auto DepthFormat = VK_FORMAT_D16_UNORM;
static constexpr auto VerticalFov = glm::radians(45.0f);
static constexpr auto NearPlane = 0.1f;
static constexpr auto CubeMapSize = 1024u;

constexpr auto mipmapCount(u32 size) {
	return u32(std::floor(std::log2(size))) + 1;
}

}
