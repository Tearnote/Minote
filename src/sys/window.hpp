// Minote - sys/window.hpp
// Wrapper for a GLFW window. An open window collects keyboard inputs
// in a thread-safe queue, and they need to be regularly collected to prevent
// the queue from filling up. The inputs need to be regularly polled to keep
// the window responsive.

#pragma once

#include <optional>
#include <atomic>
#include <mutex>
#include <GLFW/glfw3.h>
#include "base/queue.hpp"
#include "base/math.hpp"
#include "base/time.hpp"
#include "base/util.hpp"

namespace minote {

struct Window {

	// *** Windowing system interface ***

	// Whether the windowing system has been initialized
	inline static bool initialized = false;

	// Initialize the windowing system and relevant OS-specific bits. This must
	// be called once before any window is opened.
	// The thread that calls this becomes the input thread.
	static void init();

	// Clean up the windowing system. All windows must already be closed.
	// Must be called on the input thread.
	static void cleanup();

	// Collect pending events for all open windows and keep them responsive.
	// Call this as often as your target resolution of user input; at least
	// 240Hz is recommended.
	// Must be called on the input thread.
	static void poll();

	// Return the time passed since Window::init(). If init() was not called,
	// 0 is returned.
	// This function is thread-safe.
	static auto getTime() -> nsec;

	// *** Window interface ***

	// Keyboard keypress event information
	struct KeyInput {

		// GLFW keycode
		int key = GLFW_KEY_UNKNOWN;

		// GLFW_PRESS or GLFW_RELEASE
		int state = -1;

		// Time when the event was detected
		nsec timestamp = 0;
	};

	// Raw window handle. Please take note of thread safety notes in GLFW
	// documentation when using this field directly.
	GLFWwindow* handle = nullptr;

	// Mutex protecting handle access with unsynchronized GLFW functions
	mutable std::mutex handleMutex;

	// Text displayed on the window's title bar
	char const* title = nullptr;

	// Size of the window in physical pixels
	std::atomic<uvec2> size;

	// DPI scaling, where 1.0 is "standard" DPI
	std::atomic<float> scale = 0.0f;

	// Whether the window's OpenGL context is active on any thread
	bool isContextActive = false;

	// Queue containing collected keyboard inputs
	queue<KeyInput, 64> inputs;

	// Mutex protecting the queue for thread safety
	mutable std::mutex inputsMutex;

	// Open a window with specified parameters on the screen. The OpenGL context
	// is not activated by default.
	// Size of the window is in logical units. If fullscreen is true,
	// size is ignored and the window is created at desktop resolution.
	// This function must be called on the input thread.
	void open(char const* title, bool fullscreen = false, uvec2 size = {1280, 720});

	// Close an open window. The OpenGL context must be already deactivated.
	// This function must be called on the input thread.
	void close();

	// Ask if window close has been requested by application or user
	// (for example by pressing the X on the title bar). If true, the window
	// should be closed as soon as possible.
	// This function is thread-safe.
	[[nodiscard]]
	auto isClosing() const -> bool;

	// Request the window to be closed by the main thread.
	// This function is thread-safe.
	void requestClose();

	// Flip the window's front and back buffers. Call this after a frame
	// is drawn to the backbuffer to present it on the screen.
	// This function is thread-safe.
	void flip();

	// Activate the window's OpenGL context on current thread. This is required
	// before OpenGL commands can be used. A window's context can be active
	// on one thread at a time, and a thread can have only one window's context
	// active.
	// This function is thread-safe.
	void activateContext();

	// Deactivate the window's OpenGL context on current thread. The context
	// must be inactive before the window can be closed.
	// This function is thread-safe.
	void deactivateContext();

	// Remove and return a keyboard input from the window's input queue.
	// If the queue is empty, nullptr is returned. Run this often to keep
	// the queue from filling up and discarding input events.
	// This function is thread-safe.
	auto dequeueInput() -> std::optional<KeyInput>;

	// Return a keyboard input from the window's input queue without removing
	// it. If the queue is empty, nullptr is returned.
	// This function is thread-safe.
	[[nodiscard]]
	auto peekInput() const -> std::optional<KeyInput>;

	// Clear the window's input queue. This can remove a GLFW_RELEASE event,
	// so consider every key unpressed.
	// This function is thread-safe
	void clearInput();

	~Window();

};

}
