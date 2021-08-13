#include "main.hpp"

#include "config.hpp"

#include <exception>
#include <utility>
#include <thread>
#include <chrono>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#include <windows.h>
#endif //_WIN32
#include "Tracy.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/vulkan.hpp"
#include "gfx/meshes.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
#include "assets.hpp"
#include "game.hpp"

using namespace minote; // Because we can't namespace main()
using namespace base;
using namespace base::literals;

// Callback for window resize events.
static auto windowResize(void* _engine, SDL_Event* _e) -> int {
	
	// Filter for resize events only
	if (_e->type != SDL_WINDOWEVENT) return 0;
	if (_e->window.event != SDL_WINDOWEVENT_RESIZED) return 0;
	
	// Recreate swapchain and redraw
	auto newSize = uvec2{u32(_e->window.data1), u32(_e->window.data2)};
	if (newSize.x() == 0 || newSize.y() == 0) return 0; // Minimized
	auto& engine = *(gfx::Engine*)(_engine);
	engine.refreshSwapchain(newSize);
	engine.render(true);
	
	L_INFO("Window resized to {}x{}", newSize.x(), newSize.y());
	
	return 0;
	
}

auto main(int, char*[]) -> int try {
	
	tracy::SetThreadName("Input");
	
	// Initialize logging
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows terminal encoding to UTF-8
#endif //_WIN32
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}",
		AppTitle, std::get<0>(AppVersion), std::get<1>(AppVersion), std::get<2>(AppVersion));
	
	// Initialize systems
	auto system = sys::System();
	auto window = sys::Window(system, AppTitle, false, {1280, 720});
	auto vulkan = sys::Vulkan(window);
	
	// Load assets
	auto meshList = gfx::MeshList();
	auto assets = Assets(Assets_p);
	assets.loadModels([&meshList](auto name, auto data) {
		
		meshList.addGltf(name, data);
		
	});
	
	// Start up graphics engine
	auto engine = gfx::Engine(vulkan, std::move(meshList));
	
	// Initialize helpers
	auto mapper = Mapper();
	
	// Start the game thread
	auto gameThread = std::jthread(game, GameParams{
		.window = window,
		.engine = engine,
		.mapper = mapper});
	
	// Add window resize handler
	SDL_AddEventWatch(&windowResize, &engine);
	defer { SDL_DelEventWatch(&windowResize, &engine); };
	
	// Input thread loop
	while (!sys::System::isQuitting()) {
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		FrameMarkStart("Input");
		system.poll();
		FrameMarkEnd("Input");
		
	}
	
	return EXIT_SUCCESS;
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on main thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
	
}
