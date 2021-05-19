#pragma once

#include <string_view>
#include <string>
#include <atomic>
#include <mutex>
#include "base/types.hpp"
#include "base/ring.hpp"
#include "base/time.hpp"
#include "base/math.hpp"
#include "sys/keyboard.hpp"
#include "sys/glfw.hpp"

struct GLFWwindow; // Avoid GLFW header

namespace minote::sys {

using namespace base;

struct Window {

	// Keyboard keypress event information
	struct KeyInput {

		Keycode keycode;
		Scancode scancode;
		std::string_view name;
		enum struct State {
			Pressed,
			Released
		} state;
		nsec timestamp;

	};

	// Open a window with specified parameters on the screen. The OpenGL context is
	// not activated by default. Size of the window is in logical units. If fullscreen is true,
	// size is ignored and the window is created at desktop resolution.
	Window(Glfw const& glfw, std::string_view title, bool fullscreen = false, uvec2 size = {1280, 720});

	// Close the window. The OpenGL context must be already deactivated.
	~Window();

	// Window property accessors
	auto size() -> uvec2 { return m_size; }
	auto scale() -> f32 { return m_scale; }
	auto title() -> std::string_view { return m_title; }

	// Check if window close has been requested by application (via requestClose()) or
	// by the user (for example by pressing the X on the title bar). If true, the window should
	// be destroyed as soon as possible.
	// This function can be used from any thread.
	[[nodiscard]]
	auto isClosing() const -> bool;

	// Request the window to be closed.
	// This function can be used from any thread.
	void requestClose();

	// Run the provided function for every input in the queue. If the function returns false,
	// the loop is aborted and all remaining inputs (including the one for which the function
	// returned false) remain in the queue.
	template<typename F>
		requires std::predicate<F, KeyInput const&>
	void processInputs(F func);

	// Provide the raw GLFW handle. While required for certain tasks like Vulkan surface
	// creation, be careful with any operations that might require synchronization.
	auto handle() { return m_handle; }

	auto mousePos() { return m_mousePos.load(); }
	auto mouseDown() { return m_mouseDown.load(); }

	// Not movable, not copyable
	Window(Window const&) = delete;
	auto operator=(Window const&) -> Window& = delete;
	Window(Window&&) = delete;
	auto operator=(Window&&) -> Window& = delete;

private:

	static constexpr size_t InputQueueSize = 64;

	// Raw window handle
	GLFWwindow* m_handle;
	mutable std::mutex handleMutex;

	// Parent library instance
	Glfw const& glfw;

	// Text displayed on the window's title bar
	std::string m_title;

	// Queue of collected keyboard inputs
	ring<KeyInput, InputQueueSize> inputs;
	mutable std::mutex inputsMutex;

	// Size in physical pixels
	std::atomic<uvec2> m_size;

	// DPI scaling, where 1.0 is "standard" DPI
	std::atomic<f32> m_scale;

	// Current state of the mouse, for debug purposes
	std::atomic<vec2> m_mousePos;
	std::atomic<bool> m_mouseDown;

	// Function to run on each keypress event. The event is added to the queue.
	static void keyCallback(GLFWwindow*, int, int, int, int);

	// Function to run when the window is resized. The new size is kept for later retrieval,
	// such as by Frame::begin().
	static void framebufferResizeCallback(GLFWwindow*, int, int);

	// Function to run when the window is rescaled. This might happen when dragging it
	// to a display with different DPI scaling, or at startup. The new scale is saved
	// for later retrieval.
	static void windowScaleCallback(GLFWwindow*, float, float);

	static void cursorPosCallback(GLFWwindow*, double, double);

	static void mouseButtonCallback(GLFWwindow*, int, int, int);

};

}

#include "sys/window.tpp"
