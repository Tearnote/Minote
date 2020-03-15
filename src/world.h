/**
 * Keeper of the global state of the scene.
 * @file
 */

#ifndef MINOTE_WORLD_H
#define MINOTE_WORLD_H

#include "linmath/linmath.h"
#include "basetypes.h"

/**
 * Initialize world data such as lights and matrices. This must be called after
 * rendererInit().
 */
void worldInit(void);

/**
 * Cleanup world data. No other world functions can be used until worldInit()
 * is called again.
 */
void worldCleanup(void);

/**
 * Update world data. This handles independent processes such as screen resize.
 */
void worldUpdate(void);

/**
 * Set the ambient light color. This will generally be the average color
 * of the scene's background objects.
 * @param color
 */
void worldSetAmbientColor(color3 color);

vec4* worldProjection(void);

vec4* worldCamera(void);

point3f worldLightPosition(void);

color3 worldLightColor(void);

color3 worldAmbientColor(void);

#endif //MINOTE_WORLD_H
