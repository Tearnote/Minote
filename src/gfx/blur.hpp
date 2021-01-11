#pragma once

#include <vector>
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

struct GaussParams {
	std::vector<f32> coefficients;
	std::vector<f32> offsets;
};

auto generateGaussParams(int radius) -> GaussParams;

}
