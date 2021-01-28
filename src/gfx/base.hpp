#pragma once

#include <array>
#include <glm/trigonometric.hpp>
#include "volk/volk.h"
#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

static constexpr auto VulkanVersion = VK_API_VERSION_1_1;
static constexpr auto FramesInFlight = 2_zu;
static constexpr auto MaxDrawCommands = 256_zu;
static constexpr auto MaxInstances = 16'384_zu;
static constexpr auto SampleCount = VK_SAMPLE_COUNT_4_BIT;
static constexpr auto ColorFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
static constexpr auto DepthFormat = VK_FORMAT_D16_UNORM;
static constexpr auto VerticalFov = glm::radians(45.0f);
static constexpr auto NearPlane = 0.1f;
static constexpr auto FarPlane = 100.0f;

template<typename T>
using PerFrame = std::array<T, FramesInFlight>;

}
