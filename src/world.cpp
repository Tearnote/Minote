/**
 * Implementation of world.h
 * @file
 */

#include "world.hpp"

#include <assert.h>
#include "linmath/linmath.h"
#include "opengl.hpp"
#include "window.hpp"
#include "util.hpp"

using minote::rad;

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

mat4x4 worldProjection = {0}; ///< perspective transform
mat4x4 worldScreenProjection = {0}; ///< screenspace transform
mat4x4 worldCamera = {0}; ///< view transform
point3f worldLightPosition = {0}; ///< in world space
color3 worldLightColor = {0};
color3 worldAmbientColor = {0};

static size2i currentSize = {0};
static bool initialized = false;

/**
 * Ensure that matrices match the current size of the screen. This can be run
 * every frame with the current size of the screen.
 * @param size Current screen size
 */
static void worldResize(size2i size)
{
	assert(size.x > 0);
	assert(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	glViewport(0, 0, size.x, size.y);
	mat4x4_perspective(worldProjection, rad(45.0f),
		(float)size.x / (float)size.y, ProjectionNear, ProjectionFar);
	mat4x4_ortho(worldScreenProjection, 0.0f, size.x, size.y, 0.0f, 1.0f, -1.0f);
}

void worldInit(void)
{
	if (initialized) return;
	vec3 eye = {0.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(worldCamera, eye, center, up);
	worldLightPosition.x = -8.0f;
	worldLightPosition.y = 32.0f;
	worldLightPosition.z = 16.0f;
	worldLightColor.r = 1.0f;
	worldLightColor.g = 1.0f;
	worldLightColor.b = 1.0f;
	worldAmbientColor.r = 1.0f;
	worldAmbientColor.g = 1.0f;
	worldAmbientColor.b = 1.0f;
	initialized = true;
}

void worldCleanup(void)
{
	if (!initialized) return;
	initialized = false;
}

void worldUpdate(void)
{
	worldResize(windowGetSize());
}
