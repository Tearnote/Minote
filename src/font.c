// Minote - font.c

#include "font.h"

#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "util.h"
#define STBI_ASSERT(x) assert(x)
#include "stb_image/stb_image.h"

#include "util.h"
#include "log.h"

#define STR2(x) #x
#define STR(x) STR2(x)
#define CONCAT2(a, b) a ## b
#define CONCAT(a, b) CONCAT2(a, b)

#define FONT_NAME(font) STR(font)
#define FONT_JSON(font) "ttf/"STR(font)".json"
#define FONT_IMG(font) "ttf/"STR(font)".png"

#define FONT_SANS MerriweatherSans-Regular

#define FONT_SANS_NAME FONT_NAME(FONT_SANS)
#define FONT_SANS_JSON FONT_JSON(FONT_SANS)
#define FONT_SANS_IMG FONT_IMG(FONT_SANS)

#define FONT_SERIF Merriweather-Regular

#define FONT_SERIF_NAME FONT_NAME(FONT_SERIF)
#define FONT_SERIF_JSON FONT_JSON(FONT_SERIF)
#define FONT_SERIF_IMG FONT_IMG(FONT_SERIF)

struct font fonts[FontSize] = {};

static void initFont(struct font *font, const char *name, const char *json,
                     const char *atlas)
{
	strcpy(font->name, name);

	unsigned char *atlasData = NULL;
	int width;
	int height;
	int channels;
	atlasData = stbi_load(atlas, &width, &height, &channels, 4);
	if (!atlasData) {
		logCrit("Failed to load %s: %s", atlas, stbi_failure_reason());
		exit(1);
	}
	assert(width == height);
	font->atlasSize = width;

	glGenTextures(1, &font->atlas);
	glBindTexture(GL_TEXTURE_2D, font->atlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
	                GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, atlasData);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(atlasData);
}

void initFonts(void)
{
	initFont(&fonts[FontSans], FONT_SANS_NAME, FONT_SANS_JSON,
	         FONT_SANS_IMG);
	initFont(&fonts[FontSerif], FONT_SERIF_NAME, FONT_SERIF_JSON,
	         FONT_SERIF_IMG);
}

void cleanupFonts(void)
{
	for (int i = 0; i < FontSize; i++) {
		glDeleteTextures(1, &fonts[i].atlas);
		fonts[i].atlas = 0;
	}
}