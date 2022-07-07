#pragma once

#include <concepts>
#include "SDL_events.h"
#include "util/string.hpp"
#include "util/time.hpp"

namespace minote {

struct System {
	
	using Event = SDL_Event;
	
	// Initialize the windowing system, event queue, and relevant OS-specific bits.
	System();
	
	// Clean up all system bits.
	~System();
	
	// Collect pending events for all open windows and keep them responsive.
	// Call this as often as your target resolution of user input; at least
	// 240Hz is recommended.
	void poll();
	
	// Return the time passed since System() was last called.
	[[nodiscard]]
	static auto getTime() -> nsec;
	
	// Execute the provided function on each event in the queue. If the function
	// returns true, the event will be removed from the queue.
	template<typename F>
	requires std::predicate<F, Event const&>
	static void forEachEvent(F&& func) {
		
		SDL_FilterEvents([](void* f, SDL_Event* e) -> int {
			
			auto& func = *static_cast<F*>(f);
			
			if(func(*e))
				return 0;
			else
				return 1;
			
		}, &func);
		
	}
	
	// Post a synthetic quit event into the queue, signaling that the application
	// should quit.
	static void postQuitEvent();
	
	// Return true if there is a quit event in the queue.
	static auto isQuitting() -> bool;
	
	// Not copyable, not movable
	System(System const&) = delete;
	auto operator=(System const&) -> System& = delete;
	
private:
	
	// Ensure only one instance can exist
	inline static bool m_exists = false;
	
	// Frequency of the system performance counter
	inline static u64 m_timerFrequency;
	
	// Timestamp of initialization
	inline static u64 m_timerStart;
	
};

}
