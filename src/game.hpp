#pragma once

#include "sys/window.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"

namespace minote {

struct GameParams {
	
	sys::Window& window;
	gfx::Engine& engine;
	Mapper& mapper;
	
};

void game(GameParams const&);

}
