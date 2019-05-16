#include "input.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "state.h"
#include "window.h"
#include "timer.h"

#define MS_PER_UPDATE 1 // Granularity of input polling and physics updates, in milliseconds. 0 disables the limit

static uint64_t ticksPerUpdate = 0; // Target number of raw ticks between updates
static double tickRatio = 0; // Defines the ratio between raw timer ticks and nanoseconds
static uint64_t lastPollTime = 0;

void initInput() {
	ticksPerUpdate = glfwGetTimerFrequency() / (1000 * MS_PER_UPDATE);
	tickRatio = 1000000000.0 / glfwGetTimerFrequency();
}

void cleanupInput() {
	// To be continued...
}

void updateInput() {
	lastPollTime = glfwGetTimerValue();
	glfwPollEvents();
	if(glfwWindowShouldClose(window))
		setRunning(false);
}

void sleepInput() {
	uint64_t ticksPassed = glfwGetTimerValue() - lastPollTime;
	if(ticksPassed < ticksPerUpdate) // Only bother sleeping if we're ahead of the target
		sleep((long)((ticksPerUpdate - ticksPassed) * tickRatio));
}