#include "main.hpp"

#include "config.hpp"

#include <exception>
#include <cstdlib>
#include <thread>
#include <chrono>
#ifdef THREAD_DEBUG
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <processthreadsapi.h>
#endif
#include "SDL.h" // For SDL_main
#include "SDL_events.h"
#include "types.hpp"
#include "math.hpp"
#include "log.hpp"
#include "sys/system.hpp"
#include "sys/vulkan.hpp"
#include "gfx/renderer.hpp"
#include "stx/defer.hpp"
#include "game/mapper.hpp"
#include "game.hpp"

using namespace minote; // Can't namespace main()

// Window resize event handler
static auto windowResize(void*, SDL_Event* _e) -> int {
	
	// Resize events only
	if (_e->type != SDL_WINDOWEVENT) return 0;
	if (_e->window.event != SDL_WINDOWEVENT_RESIZED) return 0;
	
	// Recreate swapchain and redraw
	auto newSize = uint2{uint(_e->window.data1), uint(_e->window.data2)};
	if (newSize.x() == 0 || newSize.y() == 0) return 0; // Minimized, skip drawing entirely
	s_renderer->refreshSwapchain(newSize);
	
	L_INFO("Window resized to {}x{}", newSize.x(), newSize.y());
	
	return 0;
	
}

auto main(int, char*[]) -> int try {
#ifdef THREAD_DEBUG
	SetThreadDescription(GetCurrentThread(), L"main");
#endif
	
	// Initialize logging
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}", AppTitle, AppVersion[0], AppVersion[1], AppVersion[2]);
	
	// Initialize systems
	auto system = s_system.provide();
	auto window = s_system->openWindow(AppTitle, false, {960, 504});
	auto vulkan = s_vulkan.provide(window);
	auto engine = s_renderer.provide();
	auto mapper = game::Mapper();
	
	// Start the game thread
	auto game = Game({
		.window = window,
		.mapper = mapper,
	});
	auto gameThread = std::jthread([&] { game.run(); });
	
	// Add window resize handler
	SDL_AddEventWatch(&windowResize, &engine);
	defer { SDL_DelEventWatch(&windowResize, &engine); };
	
	// Input thread loop
	while (!s_system->isQuitting()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Forfeit thread timeslice
		s_system->poll();
	}
	
	return EXIT_SUCCESS;
	
} catch (std::exception const& e) { // Top level exception handler
	
	L_ERROR("Unhandled exception on main thread: {}", e.what());
	L_ERROR("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
	
}
