#pragma once

#include <optional>
#include <concepts>
#include <queue>
#include "base/time.hpp"
#include "sys/window.hpp"

namespace minote {

using namespace base;

struct Mapper {

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

	// Dequeue all pending keyboard inputs from the given Window, translate them to actions
	// and insert them into the actions queue. If the actions queue is full, the given Window's
	// input queue will still have all of the unprocessed inputs.
	void collectKeyInputs(sys::Window& window);

	template<typename F>
		requires std::predicate<F, Action const&>
	void processActions(F func) {
		while(!actions.empty()) {
			if(!func(actions.front())) return;
			actions.pop();
		}
	}

private:

	// Processed inputs, ready to be retrieved with peek/dequeueAction()
	std::queue<Action> actions;

};

}
