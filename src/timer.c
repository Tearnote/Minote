// Minote - timer.c

#include "timer.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <mmsystem.h>
#endif

#include <time.h>

void initTimer(void) {
#ifdef WIN32
	timeBeginPeriod(1); // Increase resolution of sleep from ~15ms to ~1.5ms
#endif
}

void cleanupTimer(void) {
#ifdef WIN32
	timeEndPeriod(1); // Restore normal sleep resolution
#endif
}

nsec getTime(void) {
	// glfwGetTime returns a double, which is ms-accurate for even longer than the maximum value of nsec
	return (nsec)(glfwGetTime() * SEC);
}

void sleep(nsec ns) {
	struct timespec duration = { .tv_sec = (long)(ns / SEC), .tv_nsec = (long)(ns % SEC) };
#ifdef WIN32
	if(duration.tv_nsec < MSEC) duration.tv_nsec = MSEC; // winpthreads cannot sleep for less than 1ms at a time
#endif
	nanosleep(&duration, NULL);
}