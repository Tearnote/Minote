#include "main.hpp"

#include "config.hpp"

#include <exception>
#include <utility>
#include <thread>
#include <chrono>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif //_WIN32
#include "base/math.hpp"
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/vulkan.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
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
	auto& engine = *static_cast<gfx::Engine*>(_engine);
	engine.refreshSwapchain(newSize);
	engine.render();
	
	L_INFO("Window resized to {}x{}", newSize.x(), newSize.y());
	
	return 0;
	
}

auto main(int, char*[]) -> int try {
	
	// Initialize logging
	
#ifdef _WIN32
	/*
	// Create console and attach standard input/output
	// https://github.com/ocaml/ocaml/issues/9252#issuecomment-576383814
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	int fdOut = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)), _O_WRONLY | _O_BINARY);
	int fdErr = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE)), _O_WRONLY | _O_BINARY);

	if (fdOut) {
		_dup2(fdOut, 1);
		_close(fdOut);
		SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(1)));
	}
	if (fdErr) {
		_dup2(fdErr, 2);
		_close(fdErr);
		SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(2)));
	}

	_dup2(_fileno(fdopen(1, "wb")), _fileno(stdout));
	_dup2(_fileno(fdopen(2, "wb")), _fileno(stderr));

	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);

	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);

	// Enable ANSI color code support
	auto out = GetStdHandle(STD_OUTPUT_HANDLE);
	auto mode = 0ul;
	GetConsoleMode(out, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(out, mode);
	*/
	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);
	
#endif //_WIN32
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}",
		AppTitle, std::get<0>(AppVersion), std::get<1>(AppVersion), std::get<2>(AppVersion));
	
	// Initialize systems
	auto system = sys::System();
	auto window = sys::Window(system, AppTitle, false, {960, 504});
	auto vulkan = sys::Vulkan(window);
	
	// Create graphics engine
	auto engine = gfx::Engine(vulkan);
	
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
		system.poll();
		
	}
	
	return EXIT_SUCCESS;
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on main thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
	
}
