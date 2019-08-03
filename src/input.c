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

fifo *inputs = NULL;
mutex inputMutex = newMutex;

#define INPUT_FREQUENCY 1000 // in Hz
#define TIME_PER_POLL (SEC / INPUT_FREQUENCY)

static nsec nextPollTime = 0;

// Function called once per every new keyboard event
static
void keyCallback(GLFWwindow *w, int key, int scancode, int action, int mods)
{
	(void)w, (void)scancode, (void)mods;

	enum inputType keyType;
	enum inputAction keyAction;
	switch (key) {
	case GLFW_KEY_LEFT:
	case GLFW_KEY_A:
		keyType = InputLeft;
		break;
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_D:
		keyType = InputRight;
		break;
	case GLFW_KEY_UP:
	case GLFW_KEY_W:
		keyType = InputUp;
		break;
	case GLFW_KEY_DOWN:
	case GLFW_KEY_S:
		keyType = InputDown;
		break;
	case GLFW_KEY_Z:
	case GLFW_KEY_J:
		keyType = InputButton1;
		break;
	case GLFW_KEY_X:
	case GLFW_KEY_K:
		keyType = InputButton2;
		break;
	case GLFW_KEY_C:
	case GLFW_KEY_L:
		keyType = InputButton3;
		break;
	case GLFW_KEY_SPACE:
		keyType = InputButton4;
		break;
	case GLFW_KEY_ESCAPE:
		keyType = InputQuit;
		break;
	case GLFW_KEY_ENTER:
		keyType = InputStart;
		break;
	default:
		return; // If it's not a key we use, instant gtfo
	}
	switch (action) {
	case GLFW_PRESS:
		keyAction = ActionPressed;
		break;
	case GLFW_RELEASE:
		keyAction = ActionReleased;
		break;
	default:
		return;
	}

	struct input *newInput = allocate(sizeof(*newInput));
	newInput->type = keyType;
	newInput->action = keyAction;
	//newInput->timestamp = nextPollTime;
	enqueueInput(newInput);
}

void initInput(void)
{
	inputs = createFifo();
	// Immediately start processing keyboard events
	glfwSetKeyCallback(window, keyCallback);
}

void cleanupInput(void)
{
	if (!inputs)
		return;
	// The FIFO might not be empty so we clear it
	struct input *i;
	while ((i = dequeueInput()))
		free(i);
	destroyFifo(inputs);
	inputs = NULL;
}

void updateInput(void)
{
	if (!nextPollTime)
		nextPollTime = getTime();

	// Get events from the system and immediately execute event callbacks
	glfwPollEvents();

	// Handle direct quit events, like the [X] being clicked
	if (glfwWindowShouldClose(window)) {
		setRunning(false);
		logInfo("Exit signal received");
	}
}

void sleepInput(void)
{
	nextPollTime += TIME_PER_POLL;
	nsec timeRemaining = nextPollTime - getTime();
	if (timeRemaining > 0)
		sleep(timeRemaining);
}
