#include "main.hpp"

#include <system_error>
#include <string_view>
#include <stdexcept>
#include <utility>
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
#include "base/version.hpp"
#include "base/assert.hpp"
#include "base/file.hpp"
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "game.hpp"

using namespace minote; // Because we can't namespace main()
using namespace base;
using namespace std::string_view_literals;

auto assertHandler(char const* expr, char const* file, int line, char const* msg) -> int {
	auto const str = fmt::format(R"(Assertion "{}" triggered on line {} in {}{}{})",
		expr, line, file, msg? ": " : "", msg?: "");
	L.crit(str);
	throw std::logic_error{str};
}

auto main(int, char*[]) -> int try {
	// *** Initialization ***

	set_assert_handler(assertHandler);

	// Unicode support
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows cmd encoding to UTF-8
#endif //_WIN32

	// Global logging
	L.level = Log::Level::Trace;
#ifndef NDEBUG
	L.console = true;
	constexpr auto Logpath = "minote-debug.log"sv;
#else //NDEBUG
	constexpr auto Logpath = "minote.log"sv;
#endif //NDEBUG
	try {
		auto logfile = file{Logpath, "w"};
		L.enableFile(std::move(logfile));
	} catch (std::system_error const& e) {
		L.warn("{}", Logpath, e.what());
	}

	L.info("Starting up {} {}", AppTitle, AppVersion);

	// Window creation
	sys::Glfw glfw;
	auto window = sys::Window{glfw, AppTitle, false, {960, 540}};

	// *** Thread startup ***

	// Game thread
	auto gameThread = std::jthread{game, std::ref(glfw), std::ref(window)};

	// Input thread loop
	while (!window.isClosing()) {
		glfw.poll();
		std::this_thread::sleep_for(1_ms);
	}

	return EXIT_SUCCESS;
} catch (std::exception const& e) {
	L.crit("Unhandled exception on main thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
}
