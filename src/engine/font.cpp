#include "engine/font.hpp"

#include <cstring>
#include <cstdio>
#include "stb/stb_image.h"
#include "base/util.hpp"

namespace minote {

void Font::create(FT_Library freetype, char const* const _name, char const* const path)
{
	ASSERT(freetype);
	ASSERT(_name);
	ASSERT(path);

	constexpr size_t MaxPathLen = 256;

	// *** Loading the Harfbuzz font ***

	char fontPath[MaxPathLen] = "";
	std::snprintf(fontPath, MaxPathLen, "%s.otf", path);

	FT_Face ftFace = nullptr;
	if (auto const err = FT_New_Face(freetype, fontPath, 0, &ftFace)) {
		L.error(R"(Failed to open font file at {} for font "{}": error {})",
			fontPath, _name, err);
		return;
	}
	defer { FT_Done_Face(ftFace); };

	if (auto const err = FT_Set_Char_Size(ftFace, 0, 1024, 0, 0)) {
		L.error(R"(Failed to set font "{}" char size: error {})", _name, err);
		return;
	}
	hbFont = hb_ft_font_create_referenced(ftFace);

	// *** Loading the atlas texture ***

	char atlasPath[MaxPathLen];
	std::snprintf(atlasPath, MaxPathLen, "%s.png", path);

	ivec2 size = {};
	int channels = 0;
	stbi_set_flip_vertically_on_load(true);
	u8* atlasData = stbi_load(atlasPath, &size.x, &size.y, &channels, 0);
	if (!atlasData) {
		L.error(R"(Failed to load font atlas at {} for font "{}": {})",
			atlasPath, _name, stbi_failure_reason());
		destroy();
		return;
	}
	defer { stbi_image_free(atlasData); };
	uvec2 const usize = size;
	size_t const atlasDataLen = usize.x * usize.y * channels;

	atlas.create(_name, usize);
	atlas.upload(atlasData, atlasDataLen, channels);

	// *** Parsing the glyph metrics ***
	char metricsPath[MaxPathLen];
	std::snprintf(metricsPath, MaxPathLen, "%s.csv", path);
	std::FILE* const metricsFile = std::fopen(metricsPath, "r");
	if (!metricsFile) {
		L.error(R"(Failed to load font metrics at {} for font "{}": {})",
			metricsPath, _name, std::strerror(errno));
		destroy();
		return;
	}
	defer { fclose(metricsFile); };

	*metrics.produce() = {}; // Empty first element
	while (true) {
		int index = 0;
		Glyph glyph = {};
		int parsed = std::fscanf(metricsFile, "%d,%*f,%f,%f,%f,%f,%f,%f,%f,%f\n",
			&index,
			&glyph.glyph.pos.x, &glyph.glyph.pos.y,
			&glyph.glyph.size.x, &glyph.glyph.size.y,
			&glyph.msdf.pos.x, &glyph.msdf.pos.y,
			&glyph.msdf.size.x, &glyph.msdf.size.y);
		if (parsed == 0 || parsed == EOF) break;
		ASSERT(static_cast<size_t>(index) == metrics.size());
		glyph.glyph.size -= glyph.glyph.pos;
		glyph.msdf.size -= glyph.msdf.pos;

		auto* const nextGlyph = metrics.produce();
		if (!nextGlyph) break;
		*nextGlyph = glyph;
	}

	name = _name;
	L.info(R"(Font "{}" loaded)", name);
}

void Font::destroy()
{
	if (hbFont) {
		hb_font_destroy(hbFont);
		hbFont = nullptr;
	}
	if (atlas.id)
		atlas.destroy();
	metrics.clear();

	L.info(R"(Font "{}" cleaned up)", stringOrNull(name));
	name = nullptr;
}

}
