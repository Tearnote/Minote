#pragma once

#include "SDL_events.h"
#include "game/mapper.hpp"
#include "math.hpp"
#include "gfx/camera.hpp"

namespace minote::game {

// A camera controller for free flying movement anywhere in the world
class Freecam {

public:
	
	// Update freecam from a mouse move event (other event types ignored)
	void handleMouse(SDL_Event const&);
	// Update freecam from a button/mousekey event
	void handleAction(Mapper::Action);
	// Apply freecam to a camera
	void updateCamera(Camera&);

private:

	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool floating = false;
	bool moving = false;
	float2 offset = {0.0f, 0.0f};
	
};

}
