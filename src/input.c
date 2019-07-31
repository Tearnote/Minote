// Minote - input.c

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
static nsec nextPollTime = 0;

// Function called once per every new keyboard event
static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) {
	(void)w, (void)scancode, (void)mods;
	
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
		default: return; // If it's not a key we use, instant gtfo
	}
	switch(action) {
		case GLFW_PRESS: keyAction = ActionPressed; break;
		case GLFW_RELEASE: keyAction = ActionReleased; break;
		default: return;
	}
	
	input* newInput = allocate(sizeof(input));
	newInput->type = keyType;
	newInput->action = keyAction;
	//newInput->timestamp = nextPollTime;
	enqueueInput(newInput);
}

void initInput(void) {
	inputs = createFifo();
	glfwSetKeyCallback(window, keyCallback); // Immediately starts processing keyboard events
}

void cleanupInput(void) {
	// The FIFO might not be empty when this function is called so we clear it
	for(input* i = dequeueInput(); !!i; i = dequeueInput())
		free(i);
	destroyFifo(inputs);
	inputs = NULL;
}

void updateInput(void) {
	if(!nextPollTime) nextPollTime = getTime();
	
	glfwPollEvents(); // Get events from the system and execute event callbacks
	if(glfwWindowShouldClose(window)) { // Handle direct quit events, like the [X] being clicked
		setRunning(false);
		logInfo("Exit signal received");
	}
}

void sleepInput(void) {
	nextPollTime += TIME_PER_POLL;
	nsec timeRemaining = nextPollTime - getTime();
	if(timeRemaining > 0) // Only bother sleeping if we're ahead of the target
		sleep(timeRemaining);
}