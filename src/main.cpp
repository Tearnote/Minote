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
#include "optick.h"
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "backends/imgui_impl_glfw.h"
#include "game.hpp"

using namespace minote; // Because we can't namespace main()
using namespace base;
using namespace base::literals;

auto main(int, char*[]) -> int try {
	
	// Initialization
	
	OPTICK_THREAD("Main");
	
#ifdef _WIN32
	SetConsoleOutputCP(65001); // Set Windows terminal encoding to UTF-8
#endif //_WIN32
	
	Log::init(Log_p, LOG_LEVEL);
	
	L_INFO("Starting up {} {}.{}.{}",
		AppTitle, std::get<0>(AppVersion), std::get<1>(AppVersion), std::get<2>(AppVersion));
	
	// Window creation
	auto glfw = sys::Glfw();
	auto window = sys::Window(glfw, AppTitle, false, {1280, 720});
	
	// Thread startup
	
	auto gameThread = std::jthread(game, std::ref(window));
	
	// Input thread loop
	while (!window.isClosing()) {
		
		glfw.poll();
		
		if (ImGui::GetIO().Fonts->IsBuilt()) // Hack
			ImGui_ImplGlfw_NewFrame();
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
	}
	
	return EXIT_SUCCESS;
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on main thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	return EXIT_FAILURE;
	
}
