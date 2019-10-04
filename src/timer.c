// Minote - timer.c

#include "timer.h"

#ifdef WIN32
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

static void yield(void)
{
#ifdef WIN32
	YieldProcessor();
#else // WIN32
	sched_yield();
#endif // WIN32
}

void sleep(nsec until)
{
	nsec diff = getTime() - until;
	if (diff > 0) {
		logDebug("Sleep target missed by %"nsecf, diff);
		return;
	}

	while (getTime() < until)
		yield();

	diff = getTime() - until;
	if (diff > MSEC)
		logDebug("Overwaited by %"nsecf, diff);
}