// Minote - engine/font.hpp
// An MDSF font resource that can be drawn with arbitrary transforms

#pragma once

#include <hb.h>
#include <hb-ft.h>
#include "base/array.hpp"
#include "base/util.hpp"
#include "base/math.hpp"
#include "sys/opengl/texture.hpp"

namespace minote {

// Loaded font data, containing a glyph atlas and metrics
struct Font {

	constexpr static size_t MaxGlyphs = 1024;

	// Size metrics of a single glyph
	struct Glyph {

		// Boundary of glyph relative to origin
		AABB<2, f32> glyph;

		// Boundary of glyph's MSDF in the atlas
		AABB<2, f32> msdf;

	};

	// Name of the font for debugging and logging purposes
	char const* name = nullptr;

	// Uploaded texture holding the atlas of MSDF glyphs
	Texture<PixelFmt::RGBA_u8> atlas;

	// Glyph metrics datasheet
	svector<Glyph, MaxGlyphs> metrics;

	// HarfBuzz font data
	hb_font_t* hbFont = nullptr;

	void create(FT_Library freetype, char const* name, char const* path);

	void destroy();

};

}
