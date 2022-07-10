#include "sys/system.hpp"

#include <cassert>

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

#include "SDL_events.h"
#include "SDL_timer.h"
#include "SDL.h"
#include "util/error.hpp"
#include "util/util.hpp"
#include "util/time.hpp"
#include "util/log.hpp"

namespace minote {

System::System() {
	
	assert(!m_exists);
	
	if (SDL_Init(
		SDL_INIT_TIMER |
		SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_HAPTIC |
		SDL_INIT_GAMECONTROLLER |
		SDL_INIT_EVENTS) != 0)
		throw runtime_error_fmt("Failed to initialize GLFW: {}", SDL_GetError());
	
	m_timerFrequency = SDL_GetPerformanceFrequency();
	m_timerStart = SDL_GetPerformanceCounter();
	
	// Increase sleep timer resolution
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw std::runtime_error("Failed to initialize Windows timer");
#endif //_WIN32
	
	m_exists = true;
	L_DEBUG("System initialized");
	
}

System::~System() {
	
#ifdef _WIN32
	timeEndPeriod(1);
#endif //_WIN32
	
	SDL_Quit();
	
	m_exists = false;
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
	
	*stdout = *fdopen(STDOUT_FILENO, "wb");
	*stderr = *fdopen(STDERR_FILENO, "wb");
	
	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
	
	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);
	
#endif
	
}

}
