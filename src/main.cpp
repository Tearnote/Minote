#include "main.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#include <windows.h>
#endif //_WIN32
#include "base/thread.hpp"
#include "base/string.hpp"
#include "base/util.hpp"
#include "base/time.hpp"
#include "base/log.hpp"
#include "base/io.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "debug.hpp"
#include "game.hpp"

using namespace minote; // Because we can't namespace main()

// Assert handler that throws critical conditions to top level.
auto assertHandler(char const* expr, char const* file, int line, char const* msg) -> int {
	auto const str{format(R"(Assertion "{}" triggered on line {} in {}{}{})",
		expr, line, file, msg? ": " : "", msg?: "")};
	L.crit(str);
	throw logic_error{str};
}

// Entry point function. Initializes systems and spawns other threads. Itself
// becomes the input handling thread. Returns EXIT_SUCCESS on successful
// execution, EXIT_FAILURE on a handled critical error, other values
// on unhandled error
auto main(int, char*[]) -> int try {
	// *** Initialization ***

	set_assert_handler(assertHandler);

	// Unicode support
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows cmd encoding to UTF-8
#endif //_WIN32

	// Global logging
#ifndef NDEBUG
	L.level = Log::Level::Trace;
	constexpr auto Logpath = "minote-debug.log"sv;
#else //NDEBUG
	L.level = Log::Level::Info;
	constexpr auto Logpath = "minote.log"sv;
#endif //NDEBUG
	L.console = true;
	try {
		file logfile{Logpath, "w"};
		L.enableFile(move(logfile));
	} catch (system_error const& e) {
		L.warn("{}", Logpath, e.what());
	}
	auto const title = format("{} {}", AppName, AppVersion);
	L.info("Starting up {}", title);

	// Window creation
	Glfw glfw;
	Window window{glfw, title};

//#ifdef MINOTE_DEBUG
	debugInputSetup(window);
//#endif //MINOTE_DEBUG

	// Thread startup
	thread gameThread(game, ref(window));

	// Signal other threads to quit if input thread terminates first
	defer { window.requestClose(); };

	// Input thread loop
	while (!window.isClosing()) {
		glfw.poll();
		sleepFor(1_ms);
	}

	return EXIT_SUCCESS;
} catch (exception const& e) {
	L.crit("Unhandled exception on main thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
}
