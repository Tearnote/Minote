#include "sys/glfw.hpp"

#include <string_view>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32
#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include "base/assert.hpp"
#include "base/time.hpp"
#include "base/log.hpp"

namespace minote::sys {

using namespace base;

Glfw::Glfw() {
	ASSERT(!exists);

	if (glfwInit() == GLFW_FALSE)
		throw std::runtime_error{fmt::format("Failed to initialize GLFW: {}", getError())};
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw std::runtime_error{"Failed to initialize Windows timer"};
#endif //_WIN32

	exists = true;
	L.debug("GLFW initialized");
}

Glfw::~Glfw() {
#ifdef _WIN32
	timeEndPeriod(1);
#endif //_WIN32
	glfwTerminate();

	exists = false;
	L.debug("GLFW cleaned up");
}

void Glfw::poll() {
	glfwPollEvents();
}

auto Glfw::getError() -> std::string_view {
	char const* description = nullptr;
	auto const code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return "No error";
	ASSERT(description);
	return description;
}

auto Glfw::getTime() -> nsec {
	return seconds(glfwGetTime());
}

auto Glfw::getKeyName(Keycode keycode, Scancode scancode) const -> std::string_view {
	return glfwGetKeyName(+keycode, +scancode)?: "Unknown";
}

}
