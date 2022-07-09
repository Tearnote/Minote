#pragma once

#include "SDL_events.h"
#include "util/math.hpp"
#include "mapper.hpp"

namespace minote {

// A camera controller for free flying movement anywhere in the world
struct Freecam {
	
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool floating = false;
	bool moving = false;
	vec2 offset = vec2(0.0f);
	
	// Update freecam from a mouse move event (other event types ignored)
	void handleMouse(SDL_Event const&);
	// Update freecam from a button/mousekey event
	void handleAction(Mapper::Action);
	// Apply freecam to the engine camera
	void updateCamera();
	
};

}
