// Minote - engine/mapper.hpp
// Converter of raw inputs from user devices into game actions

#pragma once

#include "base/array.hpp"
#include "base/ring.hpp"
#include "base/time.hpp"
#include "sys/window.hpp"

namespace minote {

// A user input event, translated to ingame action
struct Action {

	enum struct Type {
		None,
		Left, Right,
		Drop, Lock,
		RotCCW, RotCW, RotCCW2,
		Skip,
		Accept, Back,
		Size
	};

	enum struct State {
		None,
		Pressed, Released
	};

	Type type;
	State state;
	nsec timestamp;

};

struct Mapper {

	// Processed inputs, ready to be retrieved with peek/dequeueAction()
	ring<Action, 64> actions;

	// Dequeue all pending keyboard inputs from the given Window, translate
	// them to actions and insert them into the actions queue. If the actions
	// queue is full, the given Window's input queue will still have all of
	// the unprocessed inputs.
	void mapKeyInputs(Window& window);

	// Remove and return the most recent Action. If the queue is empty, nullopt
	// is returned instead.
	auto dequeueAction() -> optional<Action>;

	// Return the most recent Action without removing it. If the queue is empty,
	// nullopt is returned instead.
	[[nodiscard]]
	auto peekAction() -> optional<Action>;

};

}
