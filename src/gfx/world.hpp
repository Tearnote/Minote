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
	vec2 pad0;
	vec3 cameraPos;
	float pad1;

	vec3 sunDirection;
	float multiScatteringFactor;
	vec3 sunIlluminance;

};

}
