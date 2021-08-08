#include "sys/glfw.hpp"

#include <cassert>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32
#include "GLFW/glfw3.h"
#include "base/error.hpp"
#include "base/util.hpp"
#include "base/time.hpp"
#include "base/log.hpp"

namespace minote::sys {

using namespace base;
using namespace base::literals;

Glfw::Glfw() {
	
	assert(!m_exists);
	
	if (glfwInit() == GLFW_FALSE)
		throw runtime_error_fmt("Failed to initialize GLFW: {}", getError());
	
	// Increase sleep timer resolution
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw std::runtime_error("Failed to initialize Windows timer");
#endif //_WIN32
	
	m_exists = true;
	L_DEBUG("GLFW initialized");
	
}

Glfw::~Glfw() {
	
#ifdef _WIN32
	timeEndPeriod(1);
#endif //_WIN32
	
	glfwTerminate();
	
	m_exists = false;
	L_DEBUG("GLFW cleaned up");
	
}

void Glfw::poll() {
	
	glfwPollEvents();
	
}

auto Glfw::getError() -> string_view {
	
	auto* description = (char const*)(nullptr);
	auto code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return "No error";
	assert(description);
	return description;
	
}

auto Glfw::getTime() -> nsec {
	
	return seconds(glfwGetTime());
	
}

auto Glfw::getKeyName(Keycode _keycode, Scancode _scancode) const -> string_view {
	
	auto result = glfwGetKeyName(+_keycode, +_scancode);
	return result? result : "Unknown";
	
}

}
