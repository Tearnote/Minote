// Minote - font.c

#include "font.h"

#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

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

#define FONT_DESC(font) STR(CONCAT(font, _desc.c))
#define FONT_NAME(font) STR(font)
#define FONT_TTF(fontid) "ttf/"STR(fontid)".ttf"
#define FONT_IMG(fontid) "ttf/"STR(fontid)"_img.png"
#define FONT_INFO(fontid) CONCAT(CONCAT(font_, fontid), _information)
#define FONT_GLYPHS(fontid) CONCAT(CONCAT(font_, fontid), _codepoint_infos)
#define FONT_GLYPHCOUNT(fontid) CONCAT(CONCAT(font_, fontid), _bitmap_chars_count)

#define FONT_SANS MerriweatherSans-Regular
#define FONT_SANS_ID MerriweatherSans_Regular

#include FONT_DESC(FONT_SANS)
#define FONT_SANS_NAME FONT_NAME(FONT_SANS)
#define FONT_SANS_TTF FONT_TTF(FONT_SANS)
#define FONT_SANS_IMG FONT_IMG(FONT_SANS)
#define FONT_SANS_INFO FONT_INFO(FONT_SANS_ID)
#define FONT_SANS_GLYPHS FONT_GLYPHS(FONT_SANS_ID)
#define FONT_SANS_GLYPHCOUNT FONT_GLYPHCOUNT(FONT_SANS_ID)

#define FONT_SERIF Merriweather-Regular
#define FONT_SERIF_ID Merriweather_Regular

#include FONT_DESC(FONT_SERIF)
#define FONT_SERIF_NAME FONT_NAME(FONT_SERIF)
#define FONT_SERIF_TTF FONT_TTF(FONT_SERIF)
#define FONT_SERIF_IMG FONT_IMG(FONT_SERIF)
#define FONT_SERIF_INFO FONT_INFO(FONT_SERIF_ID)
#define FONT_SERIF_GLYPHS FONT_GLYPHS(FONT_SERIF_ID)
#define FONT_SERIF_GLYPHCOUNT FONT_GLYPHCOUNT(FONT_SERIF_ID)

FT_Library freetype;
FT_Error freetypeError;

struct font fonts[FontSize] = {};

static void initFont(struct font *font, const char *name, const char *path,
                     struct fontInfo *info, struct glyphInfo *glyphs,
                     int glyphCount, const char *atlas)
{
	strcpy(font->name, name);
	font->info = info;
	font->glyphs = glyphs;
	font->glyphCount = glyphCount;
	freetypeError = FT_New_Face(freetype, path, 0, &font->face);
	if (freetypeError) {
		logCrit("Failed to load font file %s: %s", path,
		        FT_Error_String(freetypeError));
		exit(1);
	}
	FT_Set_Char_Size(font->face, 0, 0, 0, 0);
	font->shape = hb_ft_font_create(font->face, NULL);

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, atlasData);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(atlasData);
}

void initFonts(void)
{
	freetypeError = FT_Init_FreeType(&freetype);
	if (freetypeError) {
		logCrit("Failed to initialize text renderer: %s",
		        FT_Error_String(freetypeError));
		exit(1);
	}

	initFont(&fonts[FontSans], FONT_SANS_NAME, FONT_SANS_TTF,
	         (struct fontInfo *)&FONT_SANS_INFO,
	         (struct glyphInfo *)&FONT_SANS_GLYPHS,
	         FONT_SANS_GLYPHCOUNT, FONT_SANS_IMG);

	initFont(&fonts[FontSerif], FONT_SERIF_NAME, FONT_SERIF_TTF,
	         (struct fontInfo *)&FONT_SERIF_INFO,
	         (struct glyphInfo *)&FONT_SERIF_GLYPHS,
	         FONT_SERIF_GLYPHCOUNT, FONT_SERIF_IMG);
}

void cleanupFonts(void)
{
	for (int i = 0; i < FontSize; i++) {
		glDeleteTextures(1, &fonts[i].atlas);
		fonts[i].atlas = 0;
		hb_font_destroy(fonts[i].shape);
		fonts[i].shape = NULL;
		FT_Done_Face(fonts[i].face);
		fonts[i].face = NULL;
	}

	FT_Done_FreeType(freetype);
	freetype = NULL;
}