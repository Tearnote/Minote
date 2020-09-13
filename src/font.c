/**
 * Implementation of font.h
 * @file
 */

#include "font.h"

#include <hb-ft.h>
#include "util.h"
#include "log.h"

Font fonts[FontSize] = {0};

static FT_Library freetype = null;
static bool initialized = false;

void fontInit(void)
{
	if (initialized) return;
	arrayClear(fonts);

	FT_Error err;

	err = FT_Init_FreeType(&freetype);
	if (err) {
		logError(applog, "Failed to initialize Freetype: error %d", err);
		return;
	}

	for (size_t i = FontNone + 1; i < FontSize; i += 1) {

		// Load the font file into a Harfbuzz font
		char fontPath[256] = "";
		snprintf(fontPath, 256, "%s/%s.otf", FONT_DIR, FontList[i]);
		FT_Face ftFace = null;
		err = FT_New_Face(freetype, fontPath, 0, &ftFace);
		if (err) {
			logError(applog, "Failed to open font %s (%s) with FreeType: error %d",
				FontList[i], fontPath, err);
			continue;
		}
		err = FT_Set_Char_Size(ftFace, 0, 1000, 0, 0);
		if (err) {
			logError(applog, "Failed to set font %s char size: error %d",
				FontList[i], err);
			continue;
		}
		fonts[i].hbFont = hb_ft_font_create_referenced(ftFace);
		FT_Done_Face(ftFace);
		ftFace = null;

		logInfo(applog, "Loaded font %s", FontList[i]);
	}

	initialized = true;
}

void fontCleanup(void)
{
	if (!initialized) return;

	for (size_t i = FontNone + 1; i < FontSize; i += 1) {
		hb_font_destroy(fonts[i].hbFont);
		fonts[i].hbFont = null;

		logInfo(applog, "Unloaded font %s", FontList[i]);
	}

	if (freetype) {
		FT_Done_FreeType(freetype);
		freetype = null;
	}

	initialized = false;
}
