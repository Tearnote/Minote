#pragma once

#include <string_view>
#include "base/time.hpp"
#include "sys/keyboard.hpp"

namespace minote::sys {

using namespace base;

struct Glfw {

	// Initialize the windowing system and relevant OS-specific bits.
	Glfw();

	// Clean up the windowing system.
	~Glfw();

	// Collect pending events for all open windows and keep them responsive. Call this as often
	// as your target resolution of user input; at least 240Hz is recommended.
	void poll();

	// Retrieve the description of the most recently encountered GLFW error and clear GLFW error
	// state. The description must be used before the next GLFW call.
	// This function can be used from any thread, even without a Glfw instance.
	static auto getError() -> std::string_view;

	// Return the time passed since Glfw() was last called. If it was never called, 0
	// is returned instead.
	// This function can be used from any thread.
	[[nodiscard]]
	static auto getTime() -> nsec;

	// Return the printable character that a key usually types when pressed.
	[[nodiscard]]
	auto getKeyName(Keycode, Scancode) const -> std::string_view;

	// Not copyable, not movable
	Glfw(Glfw const&) = delete;
	auto operator=(Glfw const&) -> Glfw& = delete;
	Glfw(Glfw&&) = delete;
	auto operator=(Glfw&&) -> Glfw& = delete;

private:

	// Ensure only one instance can exist
	inline static bool exists = false;

};

}
