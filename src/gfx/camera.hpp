#pragma once

#include "util/math.hpp"

namespace minote {

// A user-controllable camera. Easy to manipulate with intuitive functions,
// and can be converted into transform matrices
struct Camera {
	
	// Projection
	uint2 viewport;
	float verticalFov;
	float nearPlane;
	
	// View
	float3 position;
	float yaw;
	float pitch;
	
	// Movement
	float lookSpeed;
	float moveSpeed;
	
	// Return a unit vector of the direction the camera is pointing in
	[[nodiscard]]
	auto direction() const -> float3;
	
	// Return a matrix that transforms from world space to view space
	[[nodiscard]]
	auto view() const -> float4x4;
	
	// Return a matrix that transforms from view space to NDC space
	[[nodiscard]]
	auto projection() const -> float4x4;
	
	[[nodiscard]]
	auto viewProjection() const -> float4x4 { return mul(projection(), view()); }
	
	// Change camera direction by the provided offsets, taking into account lookSpeed
	void rotate(float horz, float vert);
	
	// Change the camera position directly, taking into account moveSpeed
	void shift(float3 distance);
	
	// Change the camera position relatively to its direction, taking into account moveSpeed
	void roam(float3 distance);
	
};

}
