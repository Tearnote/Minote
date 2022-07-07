#pragma once

#include "util/optional.hpp"
#include "SDL_events.h"

namespace minote {

// Converts events from user input devices into logical game events
struct Mapper {
	
	// A logical input event, matching the SDL_UserEvent layout
	struct Action {
		
		enum struct Type {
			None,
			Left, Right,
			Drop, Lock,
			RotCCW, RotCW, RotCCW2,
			Skip,
			Accept, Back,
			Count,
		};
		
		enum struct State {
			Pressed,
			Released,
		};
		
		Type type;
		State state;
		
	};
	
	// Convert an event into a logical game action, or return nullopt
	// if the event is irrelevant
	auto convert(SDL_Event const&) -> optional<Action>;
	
};

}
