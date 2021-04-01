#include "main.hpp"

#include <system_error>
#include <stdexcept>
#include <utility>
#include <cassert>
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
#include "base/file.hpp"
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#ifndef IMGUI_DISABLE
#include "backends/imgui_impl_glfw.h"
#endif //IMGUI_DISABLE
#include "game.hpp"

using namespace minote; // Because we can't namespace main()
using namespace base;

auto main(int, char*[]) -> int try {
	// *** Initialization ***

	// Unicode support
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows cmd encoding to UTF-8
#endif //_WIN32

	// Global logging
	L.level = Log::Level::Trace;
#ifndef NDEBUG
	L.console = true;
	constexpr auto Logpath = "minote-debug.log";
#else //NDEBUG
	constexpr auto Logpath = "minote.log";
#endif //NDEBUG
	try {
		auto logfile = file(Logpath, "w");
		L.enableFile(std::move(logfile));
	} catch (std::system_error const& e) {
		L.warn("{}", Logpath, e.what());
	}

	L.info("Starting up {} {}", AppTitle, AppVersion);

	// Window creation
	auto glfw = sys::Glfw();
	auto window = sys::Window(glfw, AppTitle, false, {960, 540});

	// *** Thread startup ***

	// Game thread
	auto gameThread = std::jthread(game, std::ref(glfw), std::ref(window));

	// Input thread loop
	while (!window.isClosing()) {
		glfw.poll();
#ifndef IMGUI_DISABLE
		if (ImGui::GetIO().Fonts->IsBuilt())
			ImGui_ImplGlfw_NewFrame();
#endif //IMGUI_DISABLE
		std::this_thread::sleep_for(1_ms);
	}

	return EXIT_SUCCESS;
} catch (std::exception const& e) {
	L.crit("Unhandled exception on main thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
}
