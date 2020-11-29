#include "main.hpp"

#include <cstdlib>
#include <thread>
#include <fmt/format.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#include <windows.h>
#endif //_WIN32
#include "base/array.hpp"
#include "base/log.hpp"
#include "sys/window.hpp"
#include "debug.hpp"
#include "game.hpp"

using namespace minote; // Because we can't namespace main()

// Entry point function. Initializes systems and spawns other threads. Itself
// becomes the input handling thread. Returns EXIT_SUCCESS on successful
// execution, EXIT_FAILURE on a handled critical error, other values
// on unhandled error
auto main(int, char*[]) -> int
{
	// *** Initialization ***

	// Unicode support
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows cmd encoding to UTF-8
#endif //_WIN32

	// Global logging
#ifndef NDEBUG
	L.level = Log::Level::Trace;
	constexpr auto Logfile = "minote-debug.log"sv;
#else //NDEBUG
	L.level = Log::Level::Info;
	constexpr auto Logfile = "minote.log"sv;
#endif //NDEBUG
	L.console = true;
	L.enableFile(Logfile);
	L.info("Starting up {} {}", AppName, AppVersion);

	// Window creation
	Window::init();
	defer { Window::cleanup(); };
	auto const windowTitle = fmt::format("{} {}", AppName, AppVersion);
	Window window;
	window.open(reinterpret_cast<char const*>(windowTitle.data()));
	defer { window.close(); };
#ifdef MINOTE_DEBUG
	debugInputSetup(window);
#endif //MINOTE_DEBUG

	// *** Thread startup ***

	// Game thread
	std::jthread gameThread(game, std::ref(window));

	// Input thread loop
	while (!window.isClosing()) {
		Window::poll();
		sleepFor(milliseconds(1));
	}

	return EXIT_SUCCESS;
}
