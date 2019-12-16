// Minote - game.h

#ifndef MINOTE_GAME_H
#define MINOTE_GAME_H

#include <memory>
#include <thread>
#include "window.h"
#include "state.h"

// A wrapper for the game thread. Constructor spawns, destructor joins
// Returns on !Window::isOpen()
class Game {
public:
	struct Inputs {
		Window& window;
		//TODO Gamepad& gamepad;
	};

	explicit Game(Inputs);
	~Game();

private:
	std::thread thread;
	Window& window;

	StateStack<Game> states;

	// Body of the thread
	auto run() -> void;
};

#endif //MINOTE_GAME_H
