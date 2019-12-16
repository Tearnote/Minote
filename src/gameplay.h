// Minote - gameplay.h

#ifndef MINOTE_GAMEPLAY_H
#define MINOTE_GAMEPLAY_H

#include "state.h"
#include "game.h"

class Gameplay : public State<Game> {
public:
	using State::State;
	auto update() -> Result override;
};

#endif //MINOTE_GAMEPLAY_H
