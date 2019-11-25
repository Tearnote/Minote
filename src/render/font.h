// Minote - render/font.h
// Loads and keeps track of fonts

#ifndef RENDER_FONT_H
#define RENDER_FONT_H

#include "glad/glad.h"

// Index for the fonts global
enum fontType {
	FontSans,
	FontSerif,
	FontMono,
	FontSize
};

struct glyphInfo {
	int x, y;
	int width, height;
	int xOffset, yOffset;
	int advance;
};

struct font {
	char name[64]; // Unimportant, only used for reference and errors
	int size;
	int glyphCount;
	GLuint atlas;
	int atlasSize;
	int atlasRange;
	struct glyphInfo *glyphs;
};

extern struct font fonts[FontSize];

void initFonts(void);
void cleanupFonts(void);

#endif //RENDER_FONT_H
