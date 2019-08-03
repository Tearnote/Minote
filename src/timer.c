// Minote - timer.c

#include "timer.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <mmsystem.h>
#endif // WIN32

#include <time.h>

void initTimer(void)
{
#ifdef WIN32
	// Increase resolution of sleep from ~15ms to ~1.5ms
	timeBeginPeriod(1);
#endif // WIN32
}

void cleanupTimer(void)
{
#ifdef WIN32
	// Restore normal sleep resolution
	timeEndPeriod(1);
#endif // WIN32
}

nsec getTime(void)
{
	// glfwGetTime returns a double, which is ms-accurate for even longer than the maximum value of nsec
	return (nsec)(glfwGetTime() * SEC);
}

void sleep(nsec ns)
{
	struct timespec duration = {};
	duration.tv_sec = (long)(ns / SEC);
	duration.tv_nsec = (long)(ns % SEC);
#ifdef WIN32
	// winpthreads cannot sleep for less than 1ms at a time
	if (duration.tv_nsec < MSEC)
		duration.tv_nsec = MSEC;
#endif // WIN32
	nanosleep(&duration, NULL);
}
