#include "store/fonts.hpp"

#include "base/log.hpp"

namespace minote {

Fonts::Fonts() noexcept {
	if (FT_Error const err = FT_Init_FreeType(&freetype))
		L.crit("Failed to initialize Freetype: error {}", err);

	fonts["jost"_id].create(freetype, "jost", "fonts/Jost-500-Medium");
}

Fonts::~Fonts() noexcept {
	fonts["jost"_id].destroy();

	if (freetype) {
		FT_Done_FreeType(freetype);
		freetype = nullptr;
	}
}

}
