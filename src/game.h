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

	std::vector<std::unique_ptr<State>> stateStack{};

	// Body of the thread
	auto run() -> void;

	auto pushState(std::unique_ptr<State>) -> void;
	// Popping state is requested by the return value of State::update() instead,
	// to prevent destroying a state object from within itself.

	auto updateStates() -> void;
	auto renderStates(Renderer&) const -> void;
};

#endif //MINOTE_GAME_H
