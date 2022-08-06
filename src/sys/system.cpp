#include "sys/system.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#endif

#include "SDL_vulkan.h"
#include "SDL_events.h"
#include "SDL_timer.h"
#include "SDL.h"
#include "util/verify.hpp"
#include "util/error.hpp"
#include "util/time.hpp"
#include "util/math.hpp"
#include "util/log.hpp"

namespace minote {

System::System() {
	
	if (SDL_Init(
		SDL_INIT_TIMER |
		SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_HAPTIC |
		SDL_INIT_GAMECONTROLLER |
		SDL_INIT_EVENTS) != 0)
		throw runtime_error_fmt("Failed to initialize SDL: {}", SDL_GetError());
	
	m_timerFrequency = SDL_GetPerformanceFrequency();
	m_timerStart = SDL_GetPerformanceCounter();
	
	// Increase sleep timer resolution
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw runtime_error("Failed to initialize Windows timer");
#endif
	
	L_DEBUG("System initialized");
	
}

System::~System() {
	
#ifdef _WIN32
	timeEndPeriod(1);
#endif
	SDL_Quit();
	L_DEBUG("System cleaned up");
	
}

void System::poll() {
	
	SDL_PumpEvents();
	
}

auto System::getTime() -> nsec {
	
	return milliseconds(SDL_GetTicks());
	
}

void System::postQuitEvent() {
	
	auto e = SDL_Event{ .quit = { .type = SDL_QUIT }};
	SDL_PushEvent(&e);
	
}

auto System::isQuitting() -> bool {
	
	return SDL_HasEvent(SDL_QUIT);
	
}

void System::initConsole() {
	
#ifdef _WIN32 // Only Windows is supported right now
	
	// https://github.com/ocaml/ocaml/issues/9252#issuecomment-576383814
	AllocConsole();
	
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	
	int fdOut = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)), _O_WRONLY | _O_BINARY);
	int fdErr = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE)),  _O_WRONLY | _O_BINARY);
	
	if (fdOut) {
		_dup2(fdOut, STDOUT_FILENO);
		_close(fdOut);
		SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(STDOUT_FILENO)));
	}
	if (fdErr) {
		_dup2(fdErr, STDERR_FILENO);
		_close(fdErr);
		SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(STDERR_FILENO)));
	}
	
	_dup2(_fileno(fdopen(STDOUT_FILENO, "wb")), _fileno(stdout));
	_dup2(_fileno(fdopen(STDERR_FILENO, "wb")), _fileno(stderr));
	
	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
	
	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);
	
#endif
	
}

Window::Window(string_view _title, bool _fullscreen, uint2 _size):
	m_title(_title) {
	
	ASSUME(_size.x() > 0 && _size.y() > 0);
	
	// Create window
	
	m_handle = SDL_CreateWindow(m_title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		_size.x(), _size.y(),
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_ALLOW_HIGHDPI |
		SDL_WINDOW_VULKAN |
		(_fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	if(!m_handle)
		throw runtime_error_fmt("Failed to create window {}: {}", m_title, SDL_GetError());
	
	// Set window properties
	
	// Real size might be different from requested size because of DPI scaling
	auto realSize = int2();
	SDL_Vulkan_GetDrawableSize(m_handle, &realSize.x(), &realSize.y());
	m_size = uint2(realSize);
	
	auto dpi = 0.0f;
	SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(m_handle), nullptr, nullptr, &dpi);
	m_dpi = dpi;
	
	L_INFO("Window {} created at {}x{}, {} DPI{}",
		m_title, realSize.x(), realSize.y(), dpi, _fullscreen? ", fullscreen" : "");
	
}

Window::~Window() {
	
	SDL_DestroyWindow(m_handle);
	L_INFO("Window {} closed", title());
	
}

}
