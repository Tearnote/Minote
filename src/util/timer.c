// Minote - timer.c

#include "timer.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

nsec getTime(void)
{
	// glfwGetTime returns a double, which is ms-accurate for even longer than the maximum value of nsec
	return (nsec)(glfwGetTime() * SEC);
}

void sleep(nsec until)
{
	nsec diff = getTime() - until;
	if (diff > 0)
		return;

	while (getTime() < until)
		; // Busywait
}