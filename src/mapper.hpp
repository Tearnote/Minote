#pragma once

#include <optional>
#include <concepts>
#include <queue>
#include "base/time.hpp"
#include "sys/window.hpp"

namespace minote {

using namespace base;

// Converter of events from user input devices into logical game events.
struct Mapper {
	
	// A logical input event
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
		nsec timestamp;
		
	};
	
	// Dequeue all pending keyboard inputs from the given Window, translate them
	// to actions and insert them into the actions queue.
	void collectKeyInputs(sys::Window&);
	
	// Execute the provided function on every action currently in the queue.
	// Processing stops prematurely if the function returns false.
	// Inlined because of a bug in MSVC.
	template<typename F>
	requires std::predicate<F, Action const&>
	void processActions(F func) {
		
		while(!m_actions.empty()) {
			
			if(!func(m_actions.front())) return;
			m_actions.pop();
			
		}
		
	}
	
private:
	
	std::queue<Action> m_actions;
	
};

}
