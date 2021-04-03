#pragma once

#include <glm/trigonometric.hpp>
#include "volk.h"

namespace minote::gfx {

static constexpr auto ColorFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
static constexpr auto DepthFormat = VK_FORMAT_D16_UNORM;
static constexpr auto VerticalFov = glm::radians(45.0f);
static constexpr auto NearPlane = 0.1f;

}
