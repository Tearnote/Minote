#pragma once

#include <string_view>
#include <optional>
#include <string>
#include <atomic>
#include <mutex>
#include <glm/vec2.hpp>
#include "base/types.hpp"
#include "base/ring.hpp"
#include "base/time.hpp"
#include "sys/keyboard.hpp"
#include "sys/glfw.hpp"

struct GLFWwindow; // Avoid GLFW header

namespace minote::sys {

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
		base::nsec timestamp;

	};

	// Open a window with specified parameters on the screen. The OpenGL context is
	// not activated by default. Size of the window is in logical units. If fullscreen is true,
	// size is ignored and the window is created at desktop resolution.
	Window(Glfw const& glfw, std::string_view title, bool fullscreen = false, glm::uvec2 size = {1280, 720});

	// Close the window. The OpenGL context must be already deactivated.
	~Window();

	// Window property accessors
	auto size() -> glm::uvec2 { return m_size; }
	auto scale() -> base::f32 { return m_scale; }
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

	// Return the oldest keyboard input from the window's input queue. If the queue is empty,
	// nullopt is returned instead.
	// This function can be used from any thread.
	[[nodiscard]]
	auto getInput() const -> std::optional<KeyInput>;

	// Remove the oldest keyboard input from the window's input queue, if any. Run this often
	// to keep the queue from filling up and discarding input events.
	// This function can be used from any thread.
	void popInput();

	// Clear the window's input queue. This can remove a key release event, so consider every
	// key unpressed.
	// This function can be used from any thread.
	void clearInput();

	// Provide the raw GLFW handle. While required for certain tasks like Vulkan surface
	// creation, be careful with any operations that might require synchronization.
	auto handle() { return m_handle; }

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
	base::ring<KeyInput, InputQueueSize> inputs;
	mutable std::mutex inputsMutex;

	// Size in physical pixels
	std::atomic<glm::uvec2> m_size;

	// DPI scaling, where 1.0 is "standard" DPI
	std::atomic<base::f32> m_scale;

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
