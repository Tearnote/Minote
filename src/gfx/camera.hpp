#pragma once

#include "util/math.hpp"

namespace minote {

// A user-controllable camera. Easy to manipulate with intuitive functions,
// and can be converted into transform matrices
struct Camera {
	
	// Projection
	uvec2 viewport;
	f32 verticalFov;
	f32 nearPlane;
	
	// View
	vec3 position;
	f32 yaw;
	f32 pitch;
	
	// Movement
	f32 lookSpeed;
	f32 moveSpeed;
	
	// Return a unit vector of the direction the camera is pointing in
	[[nodiscard]]
	auto direction() const -> vec3;
	
	// Return a matrix that transforms from world space to view space
	[[nodiscard]]
	auto view() const -> mat4;
	
	// Return a matrix that transforms from view space to NDC space
	[[nodiscard]]
	auto projection() const -> mat4;
	
	[[nodiscard]]
	auto viewProjection() const -> mat4 { return projection() * view(); }
	
	// Change camera direction by the provided offsets, taking into account lookSpeed
	void rotate(f32 horz, f32 vert);
	
	// Change the camera position directly, taking into account moveSpeed
	void shift(vec3 distance);
	
	// Change the camera position relatively to its direction, taking into account moveSpeed
	void roam(vec3 distance);
	
};

}
