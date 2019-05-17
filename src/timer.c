#include "timer.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <mmsystem.h>
#endif

#include <time.h>

void initTimer() {
#ifdef WIN32
	timeBeginPeriod(1);
#endif
}

void cleanupTimer() {
#ifdef WIN32
	timeEndPeriod(1);
#endif
}

nsec getTime() {
	return (nsec)(glfwGetTime() * SEC);
}

void sleep(nsec ns) {
	struct timespec duration = { .tv_sec = ns / SEC, .tv_nsec = ns % SEC };
#ifdef WIN32
		if(duration.tv_nsec < MSEC) duration.tv_nsec = MSEC; // winpthreads cannot wait less than 1ms at a time
#endif
	nanosleep(&duration, NULL);
}