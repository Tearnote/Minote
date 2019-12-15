// Minote - system.cpp

#include "system.h"

#include <stdexcept>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "gsl/gsl"

using namespace std::string_literals;

System::System()
{
	Expects(!exists);

	if (glfwInit() == GLFW_FALSE)
		checkError("GLFW initialization error");

	exists = true;

	Ensures(exists);
}

System::~System()
{
	if (exists) {
		glfwTerminate();
		exists = false;
	}
}

auto System::update() -> void
{
	glfwPollEvents();
}

auto System::getTime() const -> nsec
{
	return secToNsec(glfwGetTime());
}

auto System::checkError(std::string_view str) const -> void
{
	const char* errorDescription;
	int code = glfwGetError(&errorDescription);
	if (code == GLFW_NO_ERROR)
		return;

	std::string message{str};
	message += ": "s + std::to_string(code);
	if (errorDescription)
		message += " "s + errorDescription;
	throw std::runtime_error{message};
}
