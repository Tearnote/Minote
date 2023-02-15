#pragma once

#include <optional>
#include "SDL_events.h"

namespace minote::game {

// Converts events from user input devices into logical game events
class Mapper {

public:
	
	// A logical input event, matching the SDL_UserEvent layout
	struct Action {
		
		enum class Type {
			None,
			Left, Right,
			Drop, Lock,
			RotCCW, RotCW, RotCCW2,
			Skip,
			Accept, Back,
			Count,
		};
		
		enum class State {
			Pressed,
			Released,
		};
		
		Type type;
		State state;
		
	};
	
	// Convert an event into a logical game action, or return nullopt
	// if the event is irrelevant
	auto convert(SDL_Event const&) -> std::optional<Action>;
	
};

}
