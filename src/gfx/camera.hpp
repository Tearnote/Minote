#pragma once

#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

struct Camera {
	
	vec3 position;
	float yaw;
	float pitch;
	
	float lookSpeed;
	float moveSpeed;
	
	auto direction() -> vec3;
	
	auto transform() -> mat4;
	
	void rotate(float horz, float vert);
	
	void shift(vec3 distance);
	
	void roam(vec3 distance);
	
};

}
