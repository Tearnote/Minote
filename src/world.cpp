/**
 * Implementation of world.h
 * @file
 */

#include "world.hpp"

#include "sys/opengl.hpp"
#include "sys/window.hpp"
#include "base/util.hpp"

using namespace minote;

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

mat4 worldProjection = {}; ///< perspective transform
mat4 worldScreenProjection = {}; ///< screenspace transform
mat4 worldCamera = {}; ///< view transform
vec3 worldLightPosition {0}; ///< in world space
color3 worldLightColor {0};
color3 worldAmbientColor {0};

static uvec2 currentSize {0};
static bool initialized = false;

/**
 * Ensure that matrices match the current size of the screen. This can be run
 * every frame with the current size of the screen.
 * @param size Current screen size
 */
static void worldResize(uvec2 size)
{
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	glViewport(0, 0, size.x, size.y);
	worldProjection = perspective(radians(45.0f),
		(float)size.x / (float)size.y, ProjectionNear, ProjectionFar);
	worldScreenProjection = ortho(0.0f, (float)size.x, (float)size.y, 0.0f,
		1.0f, -1.0f);
}

void worldInit(void)
{
	if (initialized) return;
	constexpr vec3 eye = {0.0f, 12.0f, 32.0f};
	constexpr vec3 center = {0.0f, 12.0f, 0.0f};
	constexpr vec3 up = {0.0f, 1.0f, 0.0f};
	worldCamera = lookAt(eye, center, up);
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

void worldUpdate(Window& window)
{
	worldResize(window.size);
}
