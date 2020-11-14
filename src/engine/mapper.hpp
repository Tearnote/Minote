/**
 * Converter of raw inputs from user devices into game actions
 * @file
 */

#pragma once

#include "base/array.hpp"
#include "base/queue.hpp"
#include "base/time.hpp"
#include "sys/window.hpp"

namespace minote {

/// A user input event, translated to ingame action
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

	Type type = Type::None; ///< The virtual button related to the event
	State state = State::None; ///< What happened to the button
	nsec timestamp = 0; ///< Time when event occured

};

struct Mapper {

	queue<Action, 64> actions; ///< Processed inputs, ready to be retrieved

	/**
	 * Dequeue all pending keyboard inputs from the given input and insert
	 * them as actions to this mapper's queue.
	 * @param window Window to process. Will end up with an empty input queue,
	 * unless mapper queue is full
	 */
	void mapKeyInputs(Window& window);

	/**
	 * Remove and return the most recent Action. If the queue is empty, nothing
	 * happens.
	 * @return Most recent Action, or nullopt if queue empty
	 */
	auto dequeueAction() -> Action*;

	/**
	 * Return the most recent Action without removing it. If the queue is empty,
	 * nothing happens.
	 * @return Most recent Action, or nullopt if queue empty
	 */
	[[nodiscard]]
	auto peekAction() -> Action*;
	[[nodiscard]]
	auto peekAction() const -> const Action*;

};

}
