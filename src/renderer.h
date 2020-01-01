/**
 * Renderer object that attaches to a ::Window context and pushes triangles in
 * @file
 */

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include "window.h"
#include "log.h"
#include "color.h"

typedef struct Renderer Renderer;

/**
 * Create a new ::Renderer instance and attach it to a ::Window.
 * @param window The ::Window to attach to. Must have no other ::Renderer attached
 * @param log A ::Log to use for rendering messages
 * @return A newly created ::Renderer. Needs to be destroyed with
 * rendererDestroy()
 */
Renderer* rendererCreate(Window* window, Log* log);

/**
 * Destroy a ::Renderer instance. The context is freed from the ::Window, and
 * another ::Renderer can be attached instead. he destroyed object cannot be
 * used anymore and the pointer becomes invalid.
 * @param r The ::Renderer object
 */
void rendererDestroy(Renderer* r);

/**
 * Clear all buffers to a specified color.
 * @param r The ::Renderer object
 * @param color RGB color to clear with
 */
void rendererClear(Renderer* r, Color3 color);

/**
 * Flip buffers, presenting the rendered image to the screen.
 * @param r The ::Renderer object
 */
void rendererFlip(Renderer* r);

#endif //MINOTE_RENDERER_H
