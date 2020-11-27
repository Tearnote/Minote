#include "store/fonts.hpp"

#include "base/log.hpp"

namespace minote {

void Fonts::create()
{
	ASSERT(!freetype);

	if (FT_Error const err = FT_Init_FreeType(&freetype)) {
		L.error("Failed to initialize Freetype: error %d", err);
		return;
	}

	jost.create(freetype, "jost", "fonts/Jost-500-Medium");
}

void Fonts::destroy()
{
	ASSERT(freetype);

	jost.destroy();

	if (freetype) {
		FT_Done_FreeType(freetype);
		freetype = nullptr;
	}
}

}
