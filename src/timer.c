#include "timer.h"

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

void sleep(long ns) {
	struct timespec duration = { .tv_sec = 0, .tv_nsec = ns };
#ifdef WIN32
		if(duration.tv_nsec < 1000000) duration.tv_nsec = 1000000; // winpthreads cannot wait less than 1ms at a time, so we're clamping
#endif
	nanosleep(&duration, NULL);
}