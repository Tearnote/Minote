// Minote - font.h
// Loads and keeps track of fonts for easy access

#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include "glad/glad.h"

// We need to pretend to have *some* values, so might as well make them
// into easy numbers to deal with. Size needs to be rather large, so that
// we avoid useless features like hinting.
#define VIRTUAL_DPI 100
#define VIRTUAL_FONT_SIZE 50

// Index for the fonts global
enum fontType {
	FontSans,
	FontSerif,
	FontSize
};

// Copied from msdf_atlasgen definition
struct glyphInfo {
	unsigned int atlas_x, atlas_y;
	unsigned int atlas_w, atlas_h;
	float minx, maxx;
	float miny, maxy;
	float advance;
};

// Copied from msdf_atlasgen definition
struct fontInfo {
	unsigned int smooth_pixels;
	float min_y;
	float max_y;
	unsigned int max_height;
};

struct font {
	char name[64]; // Unimportant, only used for reference and errors
	struct fontInfo *info;
	struct glyphInfo *glyphs;
	int glyphCount;
	FT_Face face;
	hb_font_t *shape;
	GLuint atlas;
	int atlasSize;
};

extern struct font fonts[FontSize];

void initFonts(void);
void cleanupFonts(void);

#endif //FONT_H
