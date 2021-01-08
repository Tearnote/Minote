#pragma once

#include <optional>
#include "base/ring.hpp"
#include "base/time.hpp"
#include "sys/window.hpp"

namespace minote::engine {

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

	// Dequeue all pending keyboard inputs from the given Window, translate them to actions
	// and insert them into the actions queue. If the actions queue is full, the given Window's
	// input queue will still have all of the unprocessed inputs.
	void collectKeyInputs(sys::Window& window);

private:

	// Processed inputs, ready to be retrieved with peek/dequeueAction()
	ring<Action, 64> actions;

};

}
