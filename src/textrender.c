// Minote - textrender.c

#include "textrender.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "render.h"
#include "log.h"

static FT_Library freetype = NULL;

void initTextRenderer(void)
{
	FT_Error freetypeError = FT_Init_FreeType(&freetype);
	if (freetypeError) {
		logError("Failed to initialize the text renderer: %s",
		         FT_Error_String(freetypeError));
		return;
	}
}

void cleanupTextRenderer(void)
{
	if (freetype) {
		FT_Done_FreeType(freetype);
		freetype = NULL;
	}
}

void renderText(void)
{
	;
}