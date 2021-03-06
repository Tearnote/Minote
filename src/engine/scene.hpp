// Minote - engine/scene.hpp
// Description of a renderable 3D space

#pragma once

#include "base/math.hpp"

namespace minote {

// A set of values describing a 3D space that can be used for rendering in
struct Scene {

	struct Camera {

		// Position of the camera (world space)
		vec3 position = {0.0f, 12.0f, 32.0f};

		// Point the camera is looking at (world space)
		vec3 target = {0.0f, 12.0f, 0.0f};

		// Up direction, must be normalized
		vec3 up = {0.0f, 1.0f, 0.0f};

		// Near clipping plane
		f32 nearPlane = 0.1f;

		// Far clipping plane
		f32 farPlane = 100.0f;

	} camera;

	// Omnidirectional light source
	struct Light {

		// Position of the light (world space)
		vec3 position = {-8.0f, 32.0f, 16.0f};

		// Color of the light
		color3 color = {1.0f, 1.0f, 1.0f};

	} light;

	// Background color of the space
	color3 background = {1.0f, 1.0f, 1.0f};

	// The illumination applied to every lit object. Set to the average color
	// of background visuals
	color3 ambientLight = {1.0f, 1.0f, 1.0f};

	// World space -> view space transform
	mat4 view = mat4(1.0f);

	// View space -> screen space transform
	mat4 projection = mat4(1.0f);

	// Window coordinates -> screen space transform
	mat4 projection2D = mat4(1.0f);

	// Update the view, projection and projection2D matrices according
	// to current values and viewport size. Run this at the start of a frame.
	void updateMatrices(uvec2 size);

};

}
