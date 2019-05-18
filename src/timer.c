#include "timer.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <mmsystem.h>
#endif

#include <time.h>

void initTimer(void) {
#ifdef WIN32
	timeBeginPeriod(1);
#endif
}

void cleanupTimer(void) {
#ifdef WIN32
	timeEndPeriod(1);
#endif
}

nsec getTime(void) {
	return (nsec)(glfwGetTime() * SEC);
}

void sleep(nsec ns) {
	struct timespec duration = { .tv_sec = (long)(ns / SEC), .tv_nsec = (long)(ns % SEC) };
#ifdef WIN32
	if(duration.tv_nsec < MSEC) duration.tv_nsec = MSEC; // winpthreads cannot wait less than 1ms at a time
#endif
	nanosleep(&duration, NULL);
}