#pragma once

#include "util/math.hpp"

namespace minote::gfx {

// A user-controllable camera. Easy to manipulate with intuitive functions,
// and can be converted into a world->view transform matrix.
struct Camera {
	
	vec3 position;
	f32 yaw;
	f32 pitch;
	
	f32 lookSpeed;
	f32 moveSpeed;
	
	// Return a unit vector of the direction the camera is pointing in.
	[[nodiscard]]
	auto direction() const -> vec3;
	
	// Return a matrix that transforms from world space to view space.
	[[nodiscard]]
	auto transform() const -> mat4;
	
	// Change camera direction by the provided offsets, taking into account lookSpeed.
	void rotate(f32 horz, f32 vert);
	
	// Change the camera position directly, taking into account moveSpeed.
	void shift(vec3 distance);
	
	// Change the camera position relatively to its direction, taking into account moveSpeed.
	void roam(vec3 distance);
	
};

}
