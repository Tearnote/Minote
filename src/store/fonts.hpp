// Minote - store/fonts.hpp
// Storage for all available fonts

#pragma once

#include <hb-ft.h>
#include "engine/font.hpp"

namespace minote {

struct Fonts {

	FT_Library freetype = nullptr;

	Font jost;

	void create();

	void destroy();

};

}
