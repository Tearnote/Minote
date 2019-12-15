// Minote - gameplay.h

#ifndef MINOTE_GAMEPLAY_H
#define MINOTE_GAMEPLAY_H

#include "state.h"

class Gameplay : public State {
public:
	using State::State;
	auto update(bool active) -> Result override;
	auto render(bool active, Renderer& renderer) const -> void override;

private:
	void renderScene() const;
};

#endif //MINOTE_GAMEPLAY_H
