#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include "base/container/string.hpp"
#include "base/types.hpp"
#include "base/time.hpp"
#include "base/math.hpp"
#include "sys/keyboard.hpp"
#include "sys/glfw.hpp"

struct GLFWwindow; // Avoid GLFW header

namespace minote::sys {

using namespace base;

struct Window {
	
	// Keyboard keypress event
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
	
	// Open a window with specified parameters on the screen. Size of the window
	// is in logical units. If fullscreen is true, size is ignored and the window
	// is created at desktop resolution.
	Window(Glfw const&, string_view title, bool fullscreen = false, uvec2 size = {1280, 720});
	
	// Close the window. Make sure everything using the window is cleaned up.
	~Window();
	
	// Window property accessors
	[[nodiscard]]
	auto size() -> uvec2 { return m_size; }
	[[nodiscard]]
	auto scale() -> f32 { return m_scale; }
	[[nodiscard]]
	auto title() -> string_view { return m_title; }
	
	// Check if window close has been requested by application (via requestClose())
	// or by the user (for example by pressing the X on the title bar). If true,
	// the window should be destroyed as soon as possible.
	// This function can be used from any thread.
	[[nodiscard]]
	auto isClosing() const -> bool;
	
	// Request the window to be closed.
	// This function can be used from any thread.
	void requestClose();
	
	// Run the provided function for every input in the queue. If the function returns false,
	// the loop is aborted and all remaining inputs (including the one for which the function
	// returned false) remain in the queue.
	// Inlined because of a bug in MSVC.
	template<typename F>
	requires std::predicate<F, KeyInput const&>
	void processInputs(F func) {
		
		auto lock = std::scoped_lock(m_inputsMutex);
		
		while(!m_inputs.empty()) {
			
			if(!func(m_inputs.front())) return;
			m_inputs.pop();
			
		}
		
	}
	
	// Provide the raw GLFW handle. While required for certain tasks like Vulkan surface
	// creation, be careful with any operations that might require synchronization.
	[[nodiscard]]
	auto handle() -> GLFWwindow* { return m_handle; }
	
	// Mouse input, for debug UI
	[[nodiscard]]
	auto mousePos() -> vec2 { return m_mousePos.load(); }
	[[nodiscard]]
	auto mouseDown() -> bool { return m_mouseDown.load(); }
	
	// Not movable, not copyable
	Window(Window const&) = delete;
	auto operator=(Window const&) -> Window& = delete;
	
private:
	
	// Raw window handle
	GLFWwindow* m_handle;
	mutable std::mutex m_handleMutex;
	
	// Parent library instance
	Glfw const& m_glfw;
	
	// Text displayed on the window's title bar
	sstring m_title;
	
	// Queue of collected keyboard inputs
	std::queue<KeyInput> m_inputs;
	mutable std::mutex m_inputsMutex;
	
	// Size in physical pixels
	std::atomic<uvec2> m_size;
	
	// DPI scaling, where 1.0 is "standard" DPI
	std::atomic<f32> m_scale;
	
	// Current state of the mouse, for debug UI purposes
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
	
	// Cursor position callback, for debug UI purposes.
	static void cursorPosCallback(GLFWwindow*, double, double);
	
	// Mouse button callback, for debug UI purposes.
	static void mouseButtonCallback(GLFWwindow*, int, int, int);

};

}
