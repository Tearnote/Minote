#include "input.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <mmsystem.h>
#endif

#include <time.h>

#include "state.h"
#include "window.h"

#define MS_PER_UPDATE 1 // Granularity of input polling and physics updates, in milliseconds. 0 disables the limit

static uint64_t ticksPerUpdate = 0; // Target number of raw ticks between updates
static double tickRatio = 0; // Defines the ratio between raw timer ticks and nanoseconds
static uint64_t lastPollTime = 0;

void initInput() {
#ifdef WIN32
	timeBeginPeriod(1);
#endif
	ticksPerUpdate = glfwGetTimerFrequency() / (1000 * MS_PER_UPDATE);
	tickRatio = 1000000000.0 / glfwGetTimerFrequency();
}

void cleanupInput() {
#ifdef WIN32
	timeEndPeriod(1);
#endif
}

void updateInput() {
	lastPollTime = glfwGetTimerValue();
	glfwPollEvents();
	if(glfwWindowShouldClose(window))
		setRunning(false);
}

void sleepInput() {
	// Don't use non-static vars here, this runs very often
	static struct timespec pollDelay = { .tv_sec = 0, .tv_nsec = 0 };
	static uint64_t ticksPassed = 0;
	
	ticksPassed = glfwGetTimerValue() - lastPollTime;
	if(ticksPassed < ticksPerUpdate) { // Only bother sleeping if we're ahead of the target
		pollDelay.tv_nsec = (long)((ticksPerUpdate - ticksPassed) * tickRatio); // Sleep for as many ticks as we have remaining until next update, converting it to nanoseconds
#ifdef WIN32
		if(pollDelay.tv_nsec < 1000000) pollDelay.tv_nsec = 1000000; // winpthreads cannot wait less than 1ms at a time, so we're clamping
#endif
		nanosleep(&pollDelay, NULL);
	}
}