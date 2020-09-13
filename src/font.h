/**
 * MSDF font loader
 * @file
 */

#ifndef MINOTE_FONT_H
#define MINOTE_FONT_H

#include <hb.h>
#include "fontlist.h"
#include "opengl.h"
#include "darray.h"

typedef struct FontAtlasChar {
	float advance;
	float charLeft;
	float charBottom;
	float charRight;
	float charTop;
	float atlasLeft;
	float atlasBottom;
	float atlasRight;
	float atlasTop;
} FontAtlasChar;

typedef struct Font {
	Texture* atlas; ///< Uploaded texture holding the atlas of MSDF renders
	darray* metrics; ///< Array of FontAtlasChar for drawing from the atlas
	hb_font_t* hbFont; ///< Cached HarfBuzz font data for shaping
} Font;

/// Loaded font data. Initialized by fontInit() - otherwise all fields null.
/// Read-only.
extern Font fonts[FontSize];

/**
 * Load fonts into memory. Must be initialized after the renderer.
 * Must be called before any other font functions.
 */
void fontInit(void);

/**
 * Clean up the font memory. Fonts cannot be accessed until fontInit()
 * is called again.
 */
void fontCleanup(void);

#endif //MINOTE_FONT_H
