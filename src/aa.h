/**
 * Antialiasing methods, switchable at runtime
 * @file
 */

#ifndef MINOTE_AA_H
#define MINOTE_AA_H

#include "opengl.h"

/// Supported antialiasing methods
typedef enum AAMode {
	AANone, ///< zero value
	AAFast, ///< SMAA 1x
	AASimple, ///< MSAA 4x
	AAComplex, ///< SMAA S2x
	AAExtreme, ///< MSAA 16x
	AASize ///< terminator
} AAMode;

/**
 * Initialize the antialiasing system. Must be called after rendererInit(),
 * but before any other aa functions.
 * @param mode AA method to initialize
 */
void aaInit(AAMode mode);

/**
 * Cleanup the antialiasing system. No aa function can be used until aaInit()
 * is called again. Must be called before rendererCleanup().
 */
void aaCleanup(void);

/**
 * Switch from an initialized AA mode to another. Does the minimum amount of
 * regeneration, and frees GPU memory if switching to a simpler mode.
 * @param mode The new AA mode
 */
void aaSwitch(AAMode mode);

/**
 * Start rendering geometry to be antialiased into a separate framebuffer.
 */
void aaBegin(void);

/**
 * Resolve the antialiasing and blit the result of the draw to the render
 * framebuffer.
 */
void aaEnd(void);

#endif //MINOTE_AA_H
