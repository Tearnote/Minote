/**
 * MSDF font loader
 * @file
 */

#ifndef MINOTE_FONT_H
#define MINOTE_FONT_H

#include <hb.h>
#include "fontlist.hpp"
#include "opengl.hpp"
#include "base/varray.hpp"

/// Single glyph of a font alas
typedef struct FontAtlasGlyph {
	float advance; ///< x advance. Unused, HarfBuzz provides advance
	float charLeft; ///< left boundary of glyph from origin
	float charBottom; ///< bottom boundary of glyph from origin
	float charRight; ///< right boundary of glyph from origin
	float charTop; ///< top boundary of glyph from origin
	float atlasLeft; ///< left boundary of glyph in the atlas
	float atlasBottom; ///< bottom boundary of glyph in the atlas
	float atlasRight; ///< right boundary of glyph in the atlas
	float atlasTop; ///< top boundary of glyph in the atlas
} FontAtlasGlyph;

/// Complete loaded font with atlas, ready for rendering with
typedef struct Font {
	Texture* atlas; ///< Uploaded texture holding the atlas of MSDF renders
	minote::varray<FontAtlasGlyph, 1024> metrics; ///< Array of FontAtlasGlyph for drawing from the atlas
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
