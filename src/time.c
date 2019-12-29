/**
 * Implementation of time.h
 * @file
 */

#include "time.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

nsec getTime(void)
{
	return secToNsec(glfwGetTime());
}

void sleepUntil(nsec until)
{
	while (getTime() < until) {}
}
