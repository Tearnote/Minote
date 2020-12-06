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

Glfw::Glfw()
{
	DASSERT(!exists);

	if (glfwInit() == GLFW_FALSE)
		L.fail("Failed to initialize GLFW: {}", getError());
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		L.fail("Failed to initialize Windows timer");
#endif //_WIN32

	exists = true;
	L.info("Windowing initialized");
}

Glfw::~Glfw()
{
#ifdef _WIN32
	timeEndPeriod(1);
#endif //_WIN32
	glfwTerminate();

	exists = false;
	L.debug("Windowing cleaned up");
}

void Glfw::poll()
{
	glfwPollEvents();
}

auto Glfw::getError() -> string_view
{
	char const* description = nullptr;
	int const code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return "No error";
	DASSERT(description);
	return description;
}


auto Glfw::getTime() -> nsec
{
	return seconds(glfwGetTime());
}

}
