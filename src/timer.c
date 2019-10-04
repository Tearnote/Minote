// Minote - timer.c

#include "timer.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>
#else // WIN32
#include <pthread.h>
#endif // WIN32
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "log.h"

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