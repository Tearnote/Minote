#include "main.hpp"

#include "config.hpp"

#include <exception>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "SDL_events.h"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/log.hpp"
#include "sys/system.hpp"
#include "sys/vulkan.hpp"
#include "gfx/renderer.hpp"
#include "mapper.hpp"
#include "game.hpp"

using namespace minote; // Can't namespace main()

// Window resize event handler
static auto windowResize(void*, SDL_Event* _e) -> int {
	
	// Resize events only
	if (_e->type != SDL_WINDOWEVENT) return 0;
	if (_e->window.event != SDL_WINDOWEVENT_RESIZED) return 0;
	
	// Recreate swapchain and redraw
	auto newSize = uvec2{u32(_e->window.data1), u32(_e->window.data2)};
	if (newSize.x() == 0 || newSize.y() == 0) return 0; // Minimized
	s_renderer->refreshSwapchain(newSize);
	
	L_INFO("Window resized to {}x{}", newSize.x(), newSize.y());
	
	return 0;
	
}

auto main(int, char*[]) -> int try {
	
	// Initialize logging
#if SPAWN_CONSOLE
	System::initConsole();
#endif
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}", AppTitle, AppVersion[0], AppVersion[1], AppVersion[2]);
	
	// Initialize systems
	auto system = s_system.provide();
	auto window = s_system->openWindow(AppTitle, false, {960, 504});
	auto vulkan = s_vulkan.provide(window);
	auto engine = s_renderer.provide();
	auto mapper = Mapper();
	
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
	
	L_CRIT("Unhandled exception on main thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
	
}
