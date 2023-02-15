#pragma once

#include <string_view>
#include <concepts>
#include <atomic>
#include <string>
#include "SDL_events.h"
#include "SDL_video.h"
#include "util/service.hpp"
#include "types.hpp"
#include "math.hpp"
#include "util/time.hpp"

namespace minote {

// OS-specific functionality - windowing, event queue etc.
struct System {
	
	struct Window {
		
		~Window();
		
		[[nodiscard]]
		auto size() -> uint2 { return m_size; }
		[[nodiscard]]
		auto dpi() -> float { return m_dpi; }
		[[nodiscard]]
		auto title() const -> std::string_view { return m_title; }
		
		// Provide the raw SDL window handle for tasks like Vulkan surface creation
		[[nodiscard]]
		auto handle() -> SDL_Window* { return m_handle; }
		
		// Not movable, not copyable
		Window(Window const&) = delete;
		auto operator=(Window const&) -> Window& = delete;
		
	private:
		
		friend struct System;
		Window(std::string_view title, bool fullscreen, uint2 size);
		
		// Raw window handle
		SDL_Window* m_handle;
		// Text displayed on the window's title bar
		std::string m_title;
		// Size in physical pixels
		std::atomic<uint2> m_size;
		// DPI of the display the window is on
		std::atomic<float> m_dpi;
		
	};
	
	// Collect pending events for all open windows and keep them responsive.
	// Call this as often as your target resolution of user input
	void poll();
	
	// Return the time passed since this object was constructed
	[[nodiscard]]
	auto getTime() -> nsec;
	
	// Execute the provided function on each event in the queue. If the function
	// returns true, the event will be removed from the queue.
	template<typename F>
	requires std::predicate<F, SDL_Event const&>
	void forEachEvent(F&& func) {
		
		SDL_FilterEvents([](void* f, SDL_Event* e) -> int {
			auto& func = *static_cast<F*>(f);
			if(func(*e))
				return 0;
			else
				return 1;
		}, &func);
		
	}
	
	// Post a synthetic quit event into the queue, signaling
	// that the application should quit
	void postQuitEvent();
	
	// Return true if there is a quit event in the queue
	auto isQuitting() -> bool;
	
	// Create a console window and bind to standard input and output
	static void initConsole();
	
	// Open a window with specified parameters on the screen. Size of the window
	// is in logical units. If fullscreen is true, size is ignored and the window
	// is created at desktop resolution
	auto openWindow(std::string_view title, bool fullscreen = false, uint2 size = {1280, 720}) -> Window
	 { return Window(title, fullscreen, size); }
	
	System(System const&) = delete;
	auto operator=(System const&) -> System& = delete;
	
private:
	
	// Frequency of the system performance counter
	uint64 m_timerFrequency;
	
	// Timestamp of initialization
	uint64 m_timerStart;
	
	// Only usable from the service
	friend struct Service<System>;
	System();
	~System();
	
};

using Window = System::Window;

inline Service<System> s_system;

}
