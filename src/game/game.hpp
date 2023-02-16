#pragma once

#include <memory>
#include "sys/system.hpp"
#include "game/mapper.hpp"

namespace minote::game {

// Game main loop, rendering and logic
struct Game {
	
	struct Params {
		sys::Window& window;
		Mapper& mapper;
	};
	
	// Create a game on the provided window and input handler
	Game(Params const&);
	~Game();
	
	// Run the main loop of state update and rendering.
	// This is blocking, so you probably want to run this on a new thread.
	void run();
	
private:
	
	struct Impl;
	std::unique_ptr<Impl> m_impl;
	
};

}
