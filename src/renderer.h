/**
 * System for drawing inside of a window using primitives and ::Model instances
 * @file
 */

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include <stddef.h>
#include "linmath/linmath.h"
#include "basetypes.h"
#include "opengl.h"

/**
 * Initialize the renderer system. windowInit() must have been called first.
 * This can be called on a different thread than windowInit(), and the
 * calling thread becomes bound to the OpenGL context.
 */
void rendererInit(void);

/**
 * Cleanup the renderer system. No other renderer functions cannot be used
 * until rendererInit() is called again.
 */
void rendererCleanup(void);

/**
 * Return the main render ::Framebuffer. Use this if you changed from the default
 * framebuffer and want to draw to the screen again. Do not destroy or make any
 * changes to this ::Framebuffer.
 * @return The ::Framebuffer object for main rendering
 */
Framebuffer* rendererFramebuffer(void);

/**
 * Return the main render ::Texture. Use this to sample what has been drawn
 * so far. Do not destroy or make any changes to this ::Texture.
 * @return The ::Texture object for main rendering
 */
Texture* rendererTexture(void);

/**
 * Prepare for rendering a new frame. The main framebuffer is bound. You
 * should call glClear() afterwards to initialize the framebuffer
 * to a known state.
 */
void rendererFrameBegin(void);

/**
 * Present the rendered image to the screen. This call blocks until the
 * display's next vertical refresh.
 */
void rendererFrameEnd(void);

/**
 * Draws a ::Texture on top of the current framebuffer. Uses whatever blending
 * and other options are currently active, making it useful for more purposes
 * than built-in blitting.
 * @param src The ::Texture object
 * @param boost Color multiplier
 */
void rendererBlit(Texture* t, GLfloat boost);

#endif //MINOTE_RENDERER_H
