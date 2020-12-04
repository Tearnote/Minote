// Minote - sys/window.hpp
// Wrapper for a GLFW window. An open window collects keyboard inputs
// in a thread-safe queue, and they need to be regularly collected to prevent
// the queue from filling up. The inputs need to be regularly polled to keep
// the window responsive.

#pragma once

#include "base/string.hpp"
#include "base/thread.hpp"
#include "base/ring.hpp"
#include "base/math.hpp"
#include "base/time.hpp"
#include "base/util.hpp"
#include "sys/glfw.hpp"

namespace minote {

struct Window {

	// Keyboard keypress event information
	struct KeyInput {

		// GLFW keycode (GLFW_KEY_*)
		int key;

		// GLFW_PRESS or GLFW_RELEASE
		int state;

		// Time when the event was detected
		nsec timestamp;

	};

	// Raw window handle. Please take note of thread safety notes in GLFW
	// documentation if using this field directly.
	GLFWwindow* handle;

	// Mutex protecting handle access with unsynchronized GLFW functions
	mutable mutex handleMutex;

	// Text displayed on the window's title bar
	string title;

	// Size of the window in physical pixels. Safe to read from any thread
	atomic<uvec2> size;

	// DPI scaling, where 1.0 is "standard" DPI. Safe to read from any thread
	atomic<float> scale;

	// Whether the window's OpenGL context is active on any thread
	bool isContextActive;

	// Queue containing collected keyboard inputs
	ring<KeyInput, 64> inputs;

	// Mutex protecting the inputs queue for thread safety
	mutable mutex inputsMutex;

	// Open a window with specified parameters on the screen. The OpenGL context
	// is not activated by default. Size of the window is in logical units.
	// If fullscreen is true, size is ignored and the window is created
	// at desktop resolution.
	Window(Glfw const& glfw, string_view title, bool fullscreen = false, uvec2 size = {1280, 720});

	// Close the window. The OpenGL context must be already deactivated.
	~Window();

	// Check if window close has been requested by application
	// (via requestClose()) or by the user (for example by pressing the X
	// on the title bar). If true, the window should be destroyed as soon
	// as possible.
	// This function can be used from any thread.
	[[nodiscard]]
	auto isClosing() const -> bool;

	// Request the window to be closed.
	// This function can be used from any thread.
	void requestClose();

	// Flip the window's front and back buffers. Call this after a frame
	// is drawn to the backbuffer to present it on the screen.
	// This function can be used from any thread.
	void flip();

	// Activate the window's OpenGL context on current thread. This is required
	// before OpenGL commands can be used. A window's context can be active
	// on one thread at a time, and a thread can have only one window's context
	// active.
	// This function can be used from any thread.
	void activateContext();

	// Deactivate the window's OpenGL context on current thread. The context
	// must be inactive before the window can be closed.
	// This function must be used on the same thread as the window's previous
	// activateContext() call.
	void deactivateContext();

	// Remove and return a keyboard input from the window's input queue.
	// If the queue is empty, nullopt is returned. Run this often to keep
	// the queue from filling up and discarding input events.
	// This function can be used from any thread.
	auto dequeueInput() -> optional<KeyInput>;

	// Return a keyboard input from the window's input queue without removing
	// it. If the queue is empty, nullopt is returned.
	// This function can be used from any thread.
	[[nodiscard]]
	auto peekInput() const -> optional<KeyInput>;

	// Clear the window's input queue. This can remove a GLFW_RELEASE event,
	// so consider every key unpressed.
	// This function can be used from any thread.
	void clearInput();

};

}
