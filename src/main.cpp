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
#include "sys/window.hpp"
#include "sys/vulkan.hpp"
#include "sys/os.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
#include "game.hpp"

using namespace minote; // Can't namespace main()

// Window resize event handler
static auto windowResize(void* _engine, SDL_Event* _e) -> int {
	
	// Resize events only
	if (_e->type != SDL_WINDOWEVENT) return 0;
	if (_e->window.event != SDL_WINDOWEVENT_RESIZED) return 0;
	
	// Recreate swapchain and redraw
	auto newSize = uvec2{u32(_e->window.data1), u32(_e->window.data2)};
	if (newSize.x() == 0 || newSize.y() == 0) return 0; // Minimized
	auto& engine = *static_cast<Engine*>(_engine);
	engine.refreshSwapchain(newSize);
	
	L_INFO("Window resized to {}x{}", newSize.x(), newSize.y());
	
	return 0;
	
}

auto main(int, char*[]) -> int try {
	
	// Initialize logging
#if SPAWN_CONSOLE
	OS::initConsole();
#endif
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}", AppTitle, AppVersion[0], AppVersion[1], AppVersion[2]);
	
	// Initialize systems
	auto system = System();
	auto window = Window(system, AppTitle, false, {960, 504});
	auto vulkan = Vulkan(window);
	auto engine = s_engine.provide(vulkan);
	auto mapper = Mapper();
	
	// Start the game thread
	auto gameThread = std::jthread(game, GameParams{
		.window = window,
		.mapper = mapper,
	});
	
	// Add window resize handler
	SDL_AddEventWatch(&windowResize, &engine);
	defer { SDL_DelEventWatch(&windowResize, &engine); };
	
	// Input thread loop
	while (!System::isQuitting()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Forfeit thread timeslice
		system.poll();
	}
	
	return EXIT_SUCCESS;
	
} catch (std::exception const& e) { // Top level exception handler
	
	L_CRIT("Unhandled exception on main thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
	
}
