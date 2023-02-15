#pragma once

#include <memory>
#include "sys/system.hpp"
#include "game/mapper.hpp"

namespace minote {

// Game main loop, rendering and logic
struct Game {
	
	struct Params {
		Window& window;
		game::Mapper& mapper;
	};
	
	Game(Params const&);
	~Game();
	
	void run(); // Blocking; run on new thread
	
private:
	
	struct Impl;
	std::unique_ptr<Impl> m_impl;
	
};

}
