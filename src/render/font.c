// Minote - render/font.c

#include "render/font.h"

#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "util/util.h"
#define STBI_ASSERT(x) assert(x)
#include "stb_image/stb_image.h"
#include "parson/parson.h"

#include "util/util.h"
#include "util/log.h"

#define STR2(x) #x
#define STR(x) STR2(x)

#define FONT_NAME(font) STR(font)
#define FONT_JSON(font) "ttf/"STR(font)".json"
#define FONT_IMG(font) "ttf/"STR(font)".png"

#define FONT_SANS PTSans-Regular

#define FONT_SANS_NAME FONT_NAME(FONT_SANS)
#define FONT_SANS_JSON FONT_JSON(FONT_SANS)
#define FONT_SANS_IMG FONT_IMG(FONT_SANS)

#define FONT_SERIF PTSerif-Regular

#define FONT_SERIF_NAME FONT_NAME(FONT_SERIF)
#define FONT_SERIF_JSON FONT_JSON(FONT_SERIF)
#define FONT_SERIF_IMG FONT_IMG(FONT_SERIF)

#define FONT_MONO PTMono-Regular

#define FONT_MONO_NAME FONT_NAME(FONT_MONO)
#define FONT_MONO_JSON FONT_JSON(FONT_MONO)
#define FONT_MONO_IMG FONT_IMG(FONT_MONO)

struct font fonts[FontSize] = {};

static void initFont(struct font *font, const char *name, const char *json,
                     const char *atlas)
{
	strcpy(font->name, name);

	JSON_Value *root = json_parse_file(json);
	if (!root) {
		logError("Failed to parse %U", json);
		return;
	}
	JSON_Object *rootObject = json_value_get_object(root);
	font->size = (int)json_object_dotget_number(rootObject, "info.size");
	font->atlasRange = (int)json_object_dotget_number(rootObject,
	                                                  "distanceField.distanceRange");

	JSON_Array *glyphs = json_object_get_array(rootObject, "chars");
	int glyphsCount = json_array_get_count(glyphs);

	// First pass: determining the highest codepoint
	int highestCodepoint = 0;
	for (int i = 0; i < glyphsCount; i += 1) {
		JSON_Object *glyph = json_array_get_object(glyphs, i);
		int codepoint = (int)json_object_get_number(glyph, "id");
		if (codepoint > highestCodepoint)
			highestCodepoint = codepoint;
	}

	// Second pass: fill in glyph data
	font->glyphCount = highestCodepoint + 1;
	font->glyphs = allocate(sizeof(struct glyphInfo) * font->glyphCount);
	memset(font->glyphs, 0, font->glyphCount);
	for (int i = 0; i < glyphsCount; i += 1) {
		JSON_Object *glyph = json_array_get_object(glyphs, i);
		int codepoint = (int)json_object_get_number(glyph, "id");
		font->glyphs[codepoint].x =
			(int)json_object_get_number(glyph, "x");
		font->glyphs[codepoint].y =
			(int)json_object_get_number(glyph, "y");
		font->glyphs[codepoint].width =
			(int)json_object_get_number(glyph, "width");
		font->glyphs[codepoint].height =
			(int)json_object_get_number(glyph, "height");
		font->glyphs[codepoint].xOffset =
			(int)json_object_get_number(glyph, "xoffset");
		font->glyphs[codepoint].yOffset =
			(int)json_object_get_number(glyph, "yoffset");
		font->glyphs[codepoint].advance =
			(int)json_object_get_number(glyph, "xadvance");
	}

	json_value_free(root);

	unsigned char *atlasData = NULL;
	int width;
	int height;
	int channels;
	atlasData = stbi_load(atlas, &width, &height, &channels, 4);
	if (!atlasData) {
		logError("Failed to load %U: %s", atlas, stbi_failure_reason());
		return;
	}
	assert(width == height);
	font->atlasSize = width;

	glGenTextures(1, &font->atlas);
	glBindTexture(GL_TEXTURE_2D, font->atlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA,
	             GL_UNSIGNED_BYTE, atlasData);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(atlasData);
}

void initFonts(void)
{
	initFont(&fonts[FontSans], FONT_SANS_NAME, FONT_SANS_JSON,
	         FONT_SANS_IMG);
	initFont(&fonts[FontSerif], FONT_SERIF_NAME, FONT_SERIF_JSON,
	         FONT_SERIF_IMG);
	initFont(&fonts[FontMono], FONT_MONO_NAME, FONT_MONO_JSON,
	         FONT_MONO_IMG);
}

void cleanupFonts(void)
{
	for (int i = 0; i < FontSize; i += 1) {
		glDeleteTextures(1, &fonts[i].atlas);
		fonts[i].atlas = 0;
		free(fonts[i].glyphs);
		fonts[i].glyphs = NULL;
	}
}
