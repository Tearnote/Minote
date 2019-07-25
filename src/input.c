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
#include "util.h"

fifo* inputs = NULL;
mutex inputMutex = newMutex;

#define INPUT_FREQUENCY 1000 // in Hz
#define TIME_PER_POLL (SEC / INPUT_FREQUENCY)
static nsec lastPollTime = 0;

static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) {
	(void)w, (void)scancode, (void)mods;
	
	// Avoid allocating memory if the key isn't bound to anything
	inputType keyType = InputNone;
	inputAction keyAction = ActionNone;
	switch(key) {
		case GLFW_KEY_LEFT:
		case GLFW_KEY_A: keyType = InputLeft; break;
		case GLFW_KEY_RIGHT:
		case GLFW_KEY_D: keyType = InputRight; break;
		case GLFW_KEY_UP:
		case GLFW_KEY_W: keyType = InputUp; break;
		case GLFW_KEY_DOWN:
		case GLFW_KEY_S: keyType = InputDown; break;
		case GLFW_KEY_Z:
		case GLFW_KEY_J: keyType = InputButton1; break;
		case GLFW_KEY_X:
		case GLFW_KEY_K: keyType = InputButton2; break;
		case GLFW_KEY_C:
		case GLFW_KEY_L: keyType = InputButton3; break;
		case GLFW_KEY_SPACE: keyType = InputButton4; break;
		case GLFW_KEY_ESCAPE: keyType = InputQuit; break;
		case GLFW_KEY_ENTER: keyType = InputStart; break;
		default: return;
	}
	switch(action) {
		case GLFW_PRESS: keyAction = ActionPressed; break;
		case GLFW_RELEASE: keyAction = ActionReleased; break;
		default: return;
	}
	
	input* newInput = allocate(sizeof(input));
	newInput->type = keyType;
	newInput->action = keyAction;
	//newInput->timestamp = lastPollTime;
	enqueueInput(newInput);
}

void initInput(void) {
	inputs = createFifo();
	glfwSetKeyCallback(window, keyCallback);
}

void cleanupInput(void) {
	for(input* i = dequeueInput(); !!i; i = dequeueInput())
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