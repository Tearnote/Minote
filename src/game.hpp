#pragma once

#include "sys/window.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"

namespace minote {

struct GameParams {
	Window& window;
	Mapper& mapper;
};

// Entry point for the rendering + logic thread
void game(GameParams const&);

}
