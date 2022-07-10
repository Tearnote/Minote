#pragma once

#include "util/concepts.hpp"
#include "SDL_events.h"
#include "util/service.hpp"
#include "util/string.hpp"
#include "util/time.hpp"

namespace minote {

// OS-specific functionality - windowing, event queue etc.
struct System {
	
	// Collect pending events for all open windows and keep them responsive.
	// Call this as often as your target resolution of user input
	void poll();
	
	// Return the time passed since this object was constructed
	[[nodiscard]]
	auto getTime() -> nsec;
	
	// Execute the provided function on each event in the queue. If the function
	// returns true, the event will be removed from the queue.
	template<typename F>
	requires predicate<F, SDL_Event const&>
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
	
	System(System const&) = delete;
	auto operator=(System const&) -> System& = delete;
	
private:
	
	// Frequency of the system performance counter
	u64 m_timerFrequency;
	
	// Timestamp of initialization
	u64 m_timerStart;
	
	// Only usable from the service
	friend struct Service<System>;
	System();
	~System();
	
};

inline Service<System> s_system;

}
