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
		throwSystemError("GLFW initialization error");

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
	glfwWaitEvents();
}

auto System::throwSystemError(std::string_view str) -> void
{
	const char* errorDescription;
	int code = glfwGetError(&errorDescription);
	std::string message{str};
	message += ": "s + std::to_string(code);
	if (errorDescription)
		message += " "s + errorDescription;
	throw std::runtime_error{message};
}
