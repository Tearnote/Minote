#include "input.h"

#include <stdbool.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "state.h"
#include "window.h"
#include "timer.h"
#include "log.h"
#include "thread.h"
#include "fifo.h"

fifo* inputs = NULL;
mutex inputMutex = newMutex;

#define INPUT_FREQUENCY 1000 // in Hz
#define TIME_PER_POLL (SEC / INPUT_FREQUENCY)
static nsec lastPollTime = 0;

void initInput(void) {
	inputs = createFifo();
}

void cleanupInput(void) {
	for(input* i = dequeueFifo(inputs); !!i; i = dequeueFifo(inputs))
		free(i);
	destroyFifo(inputs);
	inputs = NULL;
}

void updateInput(void) {
	lastPollTime = getTime();
	glfwPollEvents();
	if(glfwWindowShouldClose(window)) {
		setRunning(false);
		logInfo("Exit signal received");
	}
}

void sleepInput(void) {
	nsec timePassed = getTime() - lastPollTime;
	if(timePassed < TIME_PER_POLL) // Only bother sleeping if we're ahead of the target
		sleep(TIME_PER_POLL - timePassed);
}