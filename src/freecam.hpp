#pragma once

#include "SDL_events.h"
#include "gfx/camera.hpp"
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
	float2 offset = {0.0f, 0.0f};
	
	// Update freecam from a mouse move event (other event types ignored)
	void handleMouse(SDL_Event const&);
	// Update freecam from a button/mousekey event
	void handleAction(Mapper::Action);
	// Apply freecam to a camera
	void updateCamera(Camera&);
	
};

}
