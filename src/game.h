// Minote - game.h

#ifndef MINOTE_GAME_H
#define MINOTE_GAME_H

#include <thread>
#include "window.h"

// A wrapper for the game thread. Constructor spawns, destructor joins
// Returns on !Window::isOpen()
class Game {
public:
	struct Inputs {
		Window& w;
	};

	explicit Game(Inputs);
	~Game();

private:
	std::thread thread;
	Window& window;

	auto run() -> void;
};

#endif //MINOTE_GAME_H
