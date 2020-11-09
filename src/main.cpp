/**
 * Game entry point
 * @file
 */

#include "main.hpp"

#include <cstdlib>
#include <clocale>
#include <thread>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#include <windows.h>
#endif //_WIN32
#include "sys/window.hpp"
#include "base/array.hpp"
#include "base/log.hpp"
#include "debug.hpp"
#include "game.hpp"

using namespace minote; // Because we can't namespace main()

/**
 * Entry point function. Initializes systems and spawns other threads. Itself
 * becomes the input handling thread.
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on a handled
 * critical error, other values on unhandled error
 */
auto main(int, char*[]) -> int
{
	// Initialization

	// Locale and Unicode
	std::setlocale(LC_ALL, ""); // Use system locale
	std::setlocale(LC_NUMERIC, "C"); // Switch to predictable number format
	std::setlocale(LC_TIME, "C"); // Switch to predictable datetime format
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows cmd encoding to UTF-8
#endif //_WIN32

	// Global logging
#ifndef NDEBUG
	L.level = Log::Level::Trace;
	constexpr char logfile[] = "minote.log";
#else //NDEBUG
	L.level = Log::Level::Info;
	constexpr char logfile[] = "minote-debug.log";
#endif //NDEBUG
	L.console = true;
	L.enableFile(logfile);
	defer { L.disableFile(); };
	L.info("Starting up %s %s", AppName, AppVersion);

	// Window creation
	Window::init();
	defer { Window::cleanup(); };
	auto const windowTitle = [] {
		array<char, 64> title = {};
		std::snprintf(title.data(), title.size(), "%s %s", AppName, AppVersion);
		return title;
	}();
	Window window;
	window.open(windowTitle.data());
	defer { window.close(); };
#ifdef MINOTE_DEBUG
	debugInputSetup(window);
#endif //MINOTE_DEBUG

	// Spawn game thread
	std::thread gameThread(game, std::ref(window));
	defer { gameThread.join(); };

	// Input loop
	while (!window.isClosing()) {
		Window::poll();
		sleepFor(milliseconds(1));
	}

	return EXIT_SUCCESS;
}
