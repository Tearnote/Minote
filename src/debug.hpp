/**
 * Layer for showing debug graphics and IMGUIs
 * @file
 */

#ifndef MINOTE_DEBUG_H
#define MINOTE_DEBUG_H

// Nuklear configuration
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

// Define this symbol to be able to change the config or include implementation
#ifndef MINOTE_NO_NUKLEAR_INCLUDE
#ifdef null
#undef null
#include "nuklear/nuklear.h"
#define null 0
#else //null
#include "nuklear/nuklear.h"
#endif //null
#endif //MINOTE_NO_NUKLEAR_INCLUDE

/// Wrap all usage of this header and Nuklear within `#ifdef MINOTE_DEBUG`.
#define MINOTE_DEBUG

/**
 * Initialize window callbacks for debug input. This will take control over
 * the mouse from the previously registered callback. It is possible to chain
 * the callbacks, but at the moment there is no need for it. This function
 * must be called from the input (main) thread.
 */
void debugInputSetup(void);

/**
 * Initialize the debug layer. Requires rendererInit() to be called first.
 * Must be called before any other debug or Nuklear functions. Allows free use
 * of Nuklear's nk_* functions to create GUI controls anywhere within
 * the same thread.
 */
void debugInit(void);

/**
 * Cleanup the debug layer. No other debug or Nuklear functions can be used
 * until debugInit() is called again.
 */
void debugCleanup(void);

/**
 * Forward latest input information to Nuklear. This should be called at
 * the same rate as debugDraw(), and before any other code has a chance
 * to use Nuklear for debug windows in the current frame.
 */
void debugUpdate(void);

/**
 * Draw all pending debug windows to the screen. This needs to be called
 * at least once per frame if Nuklear is being used.
 */
void debugDraw(void);

/**
 * Retrieve the Nuklear context for use with nk_* functions.
 * @return pointer to the Nuklear context
 */
struct nk_context* nkCtx(void);

#endif //MINOTE_DEBUG_H
