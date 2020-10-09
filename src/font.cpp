/**
 * Implementation of font.h
 * @file
 */

#include "font.hpp"

#include <hb-ft.h>
#include "stb/stb_image.h"
#include "base/util.hpp"
#include "base/log.hpp"

using namespace minote;

Font fonts[FontSize] = {0};

static FT_Library freetype = nullptr;
static bool initialized = false;

void fontInit(void)
{
	if (initialized) return;
	arrayClear(fonts);

	FT_Error err;

	err = FT_Init_FreeType(&freetype);
	if (err) {
		L.error("Failed to initialize Freetype: error %d", err);
		return;
	}

	for (size_t i = 0; i < FontSize; i += 1) {
		// Load the font file into a Harfbuzz font
		char fontPath[256] = "";
		snprintf(fontPath, 256, "%s/%s.otf", FONT_DIR, FontList[i]);
		FT_Face ftFace = nullptr;
		err = FT_New_Face(freetype, fontPath, 0, &ftFace);
		if (err) {
			L.error("Failed to open font %s (%s) with FreeType: error %d",
				FontList[i], fontPath, err);
			goto cleanup;
		}
		err = FT_Set_Char_Size(ftFace, 0, 1024, 0, 0);
		if (err) {
			L.error("Failed to set font %s char size: error %d",
				FontList[i], err);
			goto cleanup;
		}
		fonts[i].hbFont = hb_ft_font_create_referenced(ftFace);
		FT_Done_Face(ftFace);
		ftFace = nullptr;

		// Load the atlas into a texture
		{
			char atlasPath[256];
			snprintf(atlasPath, 256, "%s/%s.png", FONT_DIR, FontList[i]);
			size2i size{};
			int channels;
			stbi_set_flip_vertically_on_load(true);
			unsigned char* atlasData;
			atlasData = stbi_load(atlasPath, &size.x, &size.y, &channels, 0);
			if (!atlasData) {
				L.error("Failed to load the font atlas (%s) for font %s",
					atlasPath, FontList[i]);
				goto cleanup;
			}
			ASSERT(channels == 3);

			fonts[i].atlas.create(size, PixelFormat::RGB_f16);
			fonts[i].atlas.upload(atlasData);
			stbi_image_free(atlasData);
			atlasData = nullptr;
		}

		// Parse the csv atlas metrics
		char metricsPath[256];
		snprintf(metricsPath, 256, "%s/%s.csv", FONT_DIR, FontList[i]);
		FILE* metricsFile;
		metricsFile = fopen(metricsPath, "r");
		if (!metricsFile) {
			L.error("Failed to load font atlas metrics (%s) for font %s",
				metricsPath, FontList[i]);
			goto cleanup;
		}

		fonts[i].metrics.produce().value() = {}; // Empty first element
		while (true) {
			int index = 0;
			FontAtlasGlyph atlasChar = {0};
			int parsed = fscanf(metricsFile, "%d,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
				&index, &atlasChar.advance, &atlasChar.charLeft,
				&atlasChar.charBottom, &atlasChar.charRight, &atlasChar.charTop,
				&atlasChar.atlasLeft, &atlasChar.atlasBottom,
				&atlasChar.atlasRight, &atlasChar.atlasTop);
			if (parsed == 0 || parsed == EOF)
				break;
			ASSERT(index == fonts[i].metrics.size);

			auto nextChar = fonts[i].metrics.produce();
			if (!nextChar)
				break;
			nextChar.value() = atlasChar;
		}
		fclose(metricsFile);
		metricsFile = nullptr;

		L.info("Loaded font %s", FontList[i]);
		continue;

cleanup: // In case any stage failed, clean up all other stages
		if (fonts[i].hbFont) {
			hb_font_destroy(fonts[i].hbFont);
			fonts[i].hbFont = nullptr;
		}
		if (fonts[i].atlas.id)
			fonts[i].atlas.destroy();
	}

	initialized = true;
}

void fontCleanup(void)
{
	if (!initialized) return;

	for (size_t i = 0; i < FontSize; i += 1) {
		if (!fonts[i].hbFont && !fonts[i].atlas.id)
			continue;

		hb_font_destroy(fonts[i].hbFont);
		fonts[i].hbFont = nullptr;
		fonts[i].atlas.destroy();

		L.info("Unloaded font %s", FontList[i]);
	}

	if (freetype) {
		FT_Done_FreeType(freetype);
		freetype = nullptr;
	}

	initialized = false;
}
