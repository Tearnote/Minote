// Minote - sys/window.hpp
// Wrapper for a GLFW window. An open window collects keyboard inputs in a thread-safe queue,
// and they need to be regularly collected to prevent the queue from filling up. The inputs need
// to be regularly polled to keep the window responsive.

#pragma once

#include "base/string.hpp"
#include "base/thread.hpp"
#include "base/ring.hpp"
#include "base/math.hpp"
#include "base/time.hpp"
#include "base/util.hpp"
#include "sys/keyboard.hpp"
#include "sys/glfw.hpp"

struct GLFWwindow; // Avoid GLFW header

namespace minote {

struct Window {

	static constexpr size_t InputQueueSize{64};

	// Keyboard keypress event information
	struct KeyInput {

		Keycode keycode;
		Scancode scancode;
		string_view name;
		enum struct State {
			Pressed,
			Released
		} state;
		nsec timestamp;

	};

	// Open a window with specified parameters on the screen. The OpenGL context is
	// not activated by default. Size of the window is in logical units. If fullscreen is true,
	// size is ignored and the window is created at desktop resolution.
	Window(Glfw const& glfw, string_view title, bool fullscreen = false, uvec2 size = {1280, 720}) noexcept;

	// Close the window. The OpenGL context must be already deactivated.
	~Window();

	// Window property accessors
	auto size() -> uvec2 { return m_size; }
	auto scale() -> f32 { return m_scale; }
	auto title() -> string_view { return m_title; }

	// Check if window close has been requested by application (via requestClose()) or
	// by the user (for example by pressing the X on the title bar). If true, the window should
	// be destroyed as soon as possible.
	// This function can be used from any thread.
	[[nodiscard]]
	auto isClosing() const -> bool;

	// Request the window to be closed.
	// This function can be used from any thread.
	void requestClose();

	// Flip the window's front and back buffers. Call this after a frame is drawn
	// to the backbuffer to present it on the screen.
	// This function can be used from any thread.
	void flip();

	// Activate the window's OpenGL context on current thread. This is required before OpenGL
	// commands can be used. A window's context can be active on one thread at a time,
	// and a thread can have only one window's context active.
	// This function can be used from any thread.
	void activateContext();

	// Deactivate the window's OpenGL context on current thread. The context must be inactive
	// before the window can be closed.
	// This function must be used on the same thread as the window's previous
	// activateContext() call.
	void deactivateContext();

	// Return the oldest keyboard input from the window's input queue. If the queue is empty,
	// nullopt is returned instead.
	// This function can be used from any thread.
	[[nodiscard]]
	auto getInput() const -> optional<KeyInput>;

	// Remove the oldest keyboard input from the window's input queue, if any. Run this often
	// to keep the queue from filling up and discarding input events.
	// This function can be used from any thread.
	void popInput();

	// Clear the window's input queue. This can remove a key release event, so consider every
	// key unpressed.
	// This function can be used from any thread.
	void clearInput();

	// Not movable, not copyable
	Window(Window const&) = delete;
	auto operator=(Window const&) -> Window& = delete;
	Window(Window&&) = delete;
	auto operator=(Window&&) -> Window& = delete;

private:

	// Raw window handle
	GLFWwindow* handle;
	mutable mutex handleMutex;

	// Parent library instance
	Glfw const& glfw;

	// Text displayed on the window's title bar
	string m_title;

	// Queue of collected keyboard inputs
	ring<KeyInput, InputQueueSize> inputs;
	mutable mutex inputsMutex;

	// Size in physical pixels
	atomic<uvec2> m_size;

	// DPI scaling, where 1.0 is "standard" DPI
	atomic<f32> m_scale;

	// Whether the window's OpenGL context is active on any thread
	bool isContextActive;

	// Function to run on each keypress event. The event is added to the queue.
	static void keyCallback(GLFWwindow*, int, int, int, int);

	// Function to run when the window is resized. The new size is kept for later retrieval,
	// such as by Frame::begin().
	static void framebufferResizeCallback(GLFWwindow*, int, int);

	// Function to run when the window is rescaled. This might happen when dragging it
	// to a display with different DPI scaling, or at startup. The new scale is saved
	// for later retrieval.
	static void windowScaleCallback(GLFWwindow*, float, float);

};

}
