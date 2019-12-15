// Minote - state.h

#ifndef MINOTE_STATE_H
#define MINOTE_STATE_H

#include "renderer.h"

// Circular dependency avoidance
class Game;

// Interface for a game state class
class State {
public:
	enum Result {
		Success,
		Delete
	};

	explicit State(Game& g)
			:game{g} { }
	virtual ~State() = default;

	// Active refers to whether the state is at the top of the stack or not
	virtual auto update(bool active /*TODO Mapper&*/) -> Result = 0;
	virtual auto render(bool active, Renderer&) const -> void = 0;

private:
	// Reference back to owner, for operations on the state stack
	Game& game;
};

#endif //MINOTE_STATE_H
