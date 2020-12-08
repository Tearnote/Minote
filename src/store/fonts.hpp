// Minote - store/fonts.hpp
// Storage for all available fonts

// Currently hardcoded fonts:
// "jost"

#pragma once

#include <hb-ft.h>
#include "base/hashmap.hpp"
#include "base/string.hpp"
#include "engine/font.hpp"

namespace minote {

struct Fonts {

	// Initialize freetype and load all fonts.
	Fonts() noexcept;

	// Destroy all fonts in storage.
	~Fonts() noexcept;

	// Font access by ID
	auto operator[](ID id) -> Font& { return fonts.at(id); }
	auto operator[](ID id) const -> const Font& { return fonts.at(id); }

	// Not moveable, not copyable
	Fonts(Fonts const&) = delete;
	auto operator=(Fonts const&) -> Fonts& = delete;
	Fonts(Fonts&&) = delete;
	auto operator=(Fonts&&) -> Fonts& = delete;

private:

	// Cached library instance
	FT_Library freetype{nullptr};

	// Storage for loaded fonts
	hashmap<ID, Font> fonts;

};

}
