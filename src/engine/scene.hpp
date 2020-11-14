/**
 * Description of a renderable 3D space
 * @file
 */

#pragma once

#include "base/math.hpp"

namespace minote {

/// A set of values describing a 3D space that can be used for rendering in
struct Scene {

	struct Camera {

		vec3 position = {0.0f, 12.0f, 32.0f}; ///< Position of the camera (world space)
		vec3 target = {0.0f, 12.0f, 0.0f}; ///< Point the camera is looking at (world space)
		vec3 up = {0.0f, 1.0f, 0.0f}; ///< Up direction, must be normalized
		f32 nearPlane = 0.1f; ///< Near clipping plane
		f32 farPlane = 100.0f; ///< Far clipping plane

	} camera;

	/// Omnidirectional light source
	struct Light {

		vec3 position = {-8.0f, 32.0f, 16.0f}; ///< Position of the light (world space)
		color3 color = {1.0f, 1.0f, 1.0f};

	} light;

	color3 background = {1.0f, 1.0f, 1.0f}; ///< Color of the space (the background)
	color3 ambientLight = {1.0f, 1.0f, 1.0f}; ///< The minimum amount of illumination applied to every object

	mat4 view = mat4(1.0f); ///< World space -> view space transform
	mat4 projection = mat4(1.0f); ///< View space -> screen space transform
	mat4 projection2D = mat4(1.0f); ///< Window coordinates -> screen space transform

	/**
	 * Update the view, projection and projection2D matrices according
	 * to current values and viewport size. Run this at the start of a frame.
	 * @param size Size of the viewport in pixels
	 */
	void updateMatrices(uvec2 size);

};

}
