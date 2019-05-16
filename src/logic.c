#include "logic.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <time.h>

#include "thread.h"
#include "state.h"
#include "timer.h"

#define MS_PER_UPDATE 1 // Granularity of input polling and physics updates, in milliseconds. 0 disables the limit

static uint64_t ticksPerUpdate = 0; // Target number of raw ticks between updates
static double tickRatio = 0; // Defines the ratio between raw timer ticks and nanoseconds
static uint64_t lastPollTime = 0;

thread logicThreadID = 0;

void updateLogic() {
	// Function wins by doing absolutely nothing
}

void sleepLogic() {
	uint64_t ticksPassed = glfwGetTimerValue() - lastPollTime;
	if(ticksPassed < ticksPerUpdate) // Only bother sleeping if we're ahead of the target
		sleep((long)((ticksPerUpdate - ticksPassed) * tickRatio));
}

void* logicThread(void* param) {
	(void)param;
	
	while(isRunning()) {
		updateLogic();
		sleepLogic();
	}
	
	return NULL;
}