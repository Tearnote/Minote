/**
 * System for drawing inside of a window using primitives and ::Model instances
 * @file
 */

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include <stddef.h>
#include "base/math.hpp"
#include "sys/window.hpp"
#include "sys/opengl.hpp"

extern minote::Framebuffer renderFb;
extern minote::Texture<minote::PixelFmt::RGBA_f16> renderFbColor;
extern minote::Renderbuffer<minote::PixelFmt::DepthStencil> renderFbDepthStencil;

/**
 * Initialize the renderer system. windowInit() must have been called first.
 * This can be called on a different thread than windowInit(), and the
 * calling thread becomes bound to the OpenGL context.
 */
void rendererInit(minote::Window& window);

/**
 * Cleanup the renderer system. No other renderer functions cannot be used
 * until rendererInit() is called again.
 */
void rendererCleanup(void);

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
void rendererBlit(minote::Texture<minote::PixelFmt::RGBA_u8>& t, GLfloat boost);
void rendererBlit(minote::Texture<minote::PixelFmt::RGBA_f16>& t, GLfloat boost);

/**
 * Retrieves the current status of the synchronization feature, which prevents
 * more than one frame from being buffered in the GPU driver. This reduces video
 * latency at the cost of performance.
 * @return true for sync enabled, false for disabled
 */
bool rendererGetSync(void);

/**
 * Sets the status of the synchronization feature, which prevents more than one
 * frame from being buffered in the GPU driver. Reduces video latency
 * at the cost of performance.
 * @param enabled true for sync enabled, false for disabled
 */
void rendererSetSync(bool enabled);

#endif //MINOTE_RENDERER_H
