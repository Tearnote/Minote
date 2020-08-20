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
 * @param new ambient light color
 */
void worldSetAmbientColor(color3 color);

/**
 * Return the 3D projection matrix (view space -> screen space). Standard
 * OpenGL coordinates - Z points towards the screen.
 * @return projection matrix as 4 vectors
 */
vec4* worldProjection(void);

/**
 * Return the 2D projection matrix (window coordinates -> screen space).
 * 0,0 is top left - remember to flip Y for functions such as glScissor().
 * @return projection matrix as 4 vectors
 */
vec4* worldScreenProjection(void);

/**
 * Return the camera transform (world space -> view space).
 * @return camera matrix as 4 vectors
 */
vec4* worldCamera(void);

/**
 * Return the light source position, in world space.
 * @return light source coordinates
 */
point3f worldLightPosition(void);

/**
 * Return the light source color.
 * @return light source color
 */
color3 worldLightColor(void);

/**
 * Return the ambient light color, previously set with worldSetAmbientColor().
 * @return ambient light color
 */
color3 worldAmbientColor(void);

#endif //MINOTE_WORLD_H
