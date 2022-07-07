#pragma once

#include "sys/window.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"

namespace minote {

struct GameParams {
	
	Window& window;
	Engine& engine;
	Mapper& mapper;
	
};

void game(GameParams const&);

}
