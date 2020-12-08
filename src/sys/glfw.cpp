#include "sys/glfw.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32
#include <GLFW/glfw3.h>
#include "base/util.hpp"
#include "base/log.hpp"

namespace minote {

Glfw::Glfw() {
	ASSERT(!exists);

	if (glfwInit() == GLFW_FALSE)
		throw runtime_error{format("Failed to initialize GLFW: {}", getError())};
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw runtime_error{"Failed to initialize Windows timer"};
#endif //_WIN32

	exists = true;
	L.info("GLFW initialized");
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

auto Glfw::getError() -> string_view {
	char const* description{nullptr};
	int const code{glfwGetError(&description)};
	if (code == GLFW_NO_ERROR)
		return "No error";
	ASSERT(description);
	return description;
}

auto Glfw::getTime() -> nsec {
	return seconds(glfwGetTime());
}

auto Glfw::getKeyName(Keycode const keycode, Scancode const scancode) const -> string_view {
	return glfwGetKeyName(+keycode, +scancode)?: "Unknown";
}

}
