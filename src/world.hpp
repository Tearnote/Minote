/**
 * Keeper of the global state of the scene.
 * @file
 */

#ifndef MINOTE_WORLD_H
#define MINOTE_WORLD_H

#include "base/math.hpp"
#include "sys/window.hpp"

/// The 3D projection matrix (view space -> screen space).
/// Standard OpenGL coordinates - Z points towards the screen.
/// Read-only.
extern minote::mat4 worldProjection;

/// The 2D projection matrix (window coordinates -> screen space).
/// 0,0 is top left - remember to flip Y for functions such as glScissor().
/// Read-only.
extern minote::mat4 worldScreenProjection;

/// The camera transform (world space -> view space). Read-only.
extern minote::mat4 worldCamera;

/// Light source position, in world space. Read-write.
extern minote::vec3 worldLightPosition;

/// Light source color. Read-write.
extern minote::color3 worldLightColor;

/// Ambient light color. This will generally be the average color
/// of the scene's background objects. Read-write.
extern minote::color3 worldAmbientColor;

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
void worldUpdate(minote::Window& window);

#endif //MINOTE_WORLD_H
