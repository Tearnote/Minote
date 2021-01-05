#pragma once

#include <array>
#include "volk/volk.h"
#include "base/util.hpp"

namespace minote::engine {

using namespace base::literals;

static constexpr auto VulkanVersion = VK_API_VERSION_1_1;
static constexpr auto FramesInFlight = 2_zu;
static constexpr auto MaxDrawCommands = 256_zu;
static constexpr auto MaxInstances = 16'384_zu;
static constexpr auto SampleCount = VK_SAMPLE_COUNT_4_BIT;
//static constexpr auto ColorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
static constexpr auto DepthFormat = VK_FORMAT_D16_UNORM;

template<typename T>
using PerFrame = std::array<T, FramesInFlight>;

}
