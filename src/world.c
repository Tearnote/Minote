/**
 * Implementation of world.h
 * @file
 */

#include "world.h"

#include <assert.h>
#include "linmath/linmath.h"
#include "opengl.h"
#include "window.h"
#include "util.h"

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

static mat4x4 projection = {0}; ///< perspective transform
static mat4x4 camera = {0}; ///< view transform
static point3f lightPosition = {0}; ///< in world space
static color3 lightColor = {0};
static color3 ambientColor = {0};

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
	mat4x4_perspective(projection, radf(45.0f),
		(float)size.x / (float)size.y, ProjectionNear, ProjectionFar);
}

void worldInit(void)
{
	if (initialized) return;
	vec3 eye = {4.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(camera, eye, center, up);
	lightPosition.x = -8.0f;
	lightPosition.y = 32.0f;
	lightPosition.z = 16.0f;
	lightColor.r = 1.0f;
	lightColor.g = 1.0f;
	lightColor.b = 1.0f;
	ambientColor.r = 1.0f;
	ambientColor.g = 1.0f;
	ambientColor.b = 1.0f;
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

void worldSetAmbientColor(color3 color)
{
	color3Copy(ambientColor, color);
}

vec4* worldProjection(void)
{
	return projection;
}

vec4* worldCamera(void)
{
	return camera;
}

point3f worldLightPosition(void)
{
	return lightPosition;
}

color3 worldLightColor(void)
{
	return lightColor;
}

color3 worldAmbientColor(void)
{
	return ambientColor;
}
