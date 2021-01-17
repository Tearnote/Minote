#pragma once

#include <vector>
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

struct GaussSample {

	float weight;
	float offset;
};

auto generateGaussParams(int radius) -> std::vector<GaussSample>;

}
