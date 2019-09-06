// Minote - font.h
// Loads and keeps track of fonts

#ifndef FONT_H
#define FONT_H

#include "glad/glad.h"

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
	GLuint atlas;
	int atlasSize;
};

extern struct font fonts[FontSize];

void initFonts(void);
void cleanupFonts(void);

#endif //FONT_H
