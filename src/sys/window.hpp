#pragma once

#include <atomic>
#include "util/string.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "sys/system.hpp"

// Forward declaration
struct SDL_Window;

namespace minote {

struct Window {
	
	// Open a window with specified parameters on the screen. Size of the window
	// is in logical units. If fullscreen is true, size is ignored and the window
	// is created at desktop resolution.
	Window(string_view title, bool fullscreen = false, uvec2 size = {1280, 720});
	
	// Close the window. Make sure everything using the window is cleaned up.
	~Window();
	
	// Window property accessors
	[[nodiscard]]
	auto size() -> uvec2 { return m_size; }
	[[nodiscard]]
	auto dpi() -> f32 { return m_dpi; }
	[[nodiscard]]
	auto title() const -> string_view { return m_title; }
	
	// Provide the raw GLFW handle. While required for certain tasks like Vulkan surface
	// creation, be careful with any operations that might require synchronization.
	[[nodiscard]]
	auto handle() -> SDL_Window* { return m_handle; }
	
	// Not movable, not copyable
	Window(Window const&) = delete;
	auto operator=(Window const&) -> Window& = delete;
	
private:
	
	// Raw window handle
	SDL_Window* m_handle;
	
	// Text displayed on the window's title bar
	string m_title;
	
	// Size in physical pixels
	std::atomic<uvec2> m_size;
	
	// DPI of the display the window is on
	std::atomic<f32> m_dpi;
	
};

}
