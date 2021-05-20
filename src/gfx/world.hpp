#pragma once

#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

struct World {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat4 viewProjectionInverse;
	uvec2 viewportSize;
};

}
