// Minote - main/poll.c

#include "main/poll.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "readall/readall.h"

#include "util/log.h"
#include "util/util.h"
#include "util/timer.h"
#include "global/state.h"
#include "global/input.h"
#include "main/window.h"

#define INPUT_FREQUENCY 500 // in Hz
#define TIME_PER_POLL (SEC / INPUT_FREQUENCY)

#define MAPPINGS_PATH "conf/gamepad/gamecontrollerdb.txt"

static nsec nextPollTime = -1;

static bool gamepads[GLFW_JOYSTICK_LAST + 1] = {};
static GLFWgamepadstate gamepadStates[GLFW_JOYSTICK_LAST + 1] = {};

#define ANALOG_DEADZONE 0.4

static void generateInput(enum inputType type, enum inputAction action)
{
	struct input *newInput = allocate(sizeof(*newInput));
	newInput->type = type;
	newInput->action = action;
	enqueueInput(newInput);
}

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

	generateInput(keyType, keyAction);
}

static void enumerateGamepads(void)
{
	for (int i = 0; i <= GLFW_JOYSTICK_LAST; i += 1) {
		// Gamepad = xinput mapping
		bool detected = glfwJoystickIsGamepad(i);
		if (gamepads[i] && !detected) {
			gamepads[i] = detected;
			logInfo("Gamepad #%d disconnected", i);
			continue;
		}
		if (detected && !gamepads[i]) {
			gamepads[i] = detected;
			memset(&gamepadStates[i], 0, sizeof(gamepadStates[i]));
			logInfo("Gamepad #%d connected: %s", i,
			        glfwGetGamepadName(i));
			continue;
		}
		// Joystick = no xinput mapping
		if (!detected && !gamepads[i] && glfwJoystickPresent(i))
			logWarn("Unsupported joystick #%d connected: %s", i,
			        glfwGetJoystickName(i));
	}
}

// Naively reenumerate all joysticks on any device change
static void joystickCallback(int jid, int event)
{
	(void)jid, (void)event;
	enumerateGamepads();
}

void initPoll(void)
{
	FILE *mappingsFile = fopen(MAPPINGS_PATH, "r");
	if (mappingsFile == NULL) {
		logCrit("Could not open %U for reading: %s\n", MAPPINGS_PATH,
		        strerror(errno));
		exit(EXIT_FAILURE);
	}
	char *mappings = NULL;
	size_t mappingsChars = 0;
	if (readall(mappingsFile, &mappings, &mappingsChars) != READALL_OK) {
		logCrit("Could not read contents of %U\n", MAPPINGS_PATH);
		exit(EXIT_FAILURE);
	}
	glfwUpdateGamepadMappings(mappings);
	free(mappings);
	mappings = NULL;
	fclose(mappingsFile);
	mappingsFile = NULL;
	glfwSetJoystickCallback(joystickCallback);
	enumerateGamepads();

	// Immediately start processing keyboard events
	glfwSetKeyCallback(window, keyCallback);
}

void cleanupPoll(void)
{
	// Huh.
}

static void pollGamepadEvents(void)
{
	for (int i = 0; i <= GLFW_JOYSTICK_LAST; i += 1) {
		if (!gamepads[i])
			continue;
		GLFWgamepadstate newState;
		glfwGetGamepadState(i, &newState);
		for (int j = 0; j < countof(newState.buttons); j += 1) {
			if (gamepadStates[i].buttons[j] == newState.buttons[j])
				continue; // No change in button state
			gamepadStates[i].buttons[j] = newState.buttons[j];

			enum inputType keyType;
			enum inputAction keyAction;

			switch (j) {
			case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
				keyType = InputLeft;
				break;
			case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
				keyType = InputRight;
				break;
			case GLFW_GAMEPAD_BUTTON_DPAD_UP:
				keyType = InputUp;
				break;
			case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
				keyType = InputDown;
				break;
			case GLFW_GAMEPAD_BUTTON_A:
				keyType = InputButton1;
				break;
			case GLFW_GAMEPAD_BUTTON_B:
				keyType = InputButton2;
				break;
			case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:
				keyType = InputButton3;
				break;
			case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:
				keyType = InputButton4;
				break;
			case GLFW_GAMEPAD_BUTTON_START:
				keyType = InputStart;
				break;
			case GLFW_GAMEPAD_BUTTON_BACK:
				keyType = InputQuit;
				break;
			default:
				keyType = InputNone;
			}

			if (newState.buttons[j] == GLFW_PRESS)
				keyAction = ActionPressed;
			else
				keyAction = ActionReleased;

			generateInput(keyType, keyAction);
		}

		// Analog stick 8-way emulation
		float oldX = gamepadStates[i].axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		float oldY = gamepadStates[i].axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		float newX = newState.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		float newY = newState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		gamepadStates[i].axes[GLFW_GAMEPAD_AXIS_LEFT_X] =
			newState.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		gamepadStates[i].axes[GLFW_GAMEPAD_AXIS_LEFT_Y] =
			newState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		if (newX < -ANALOG_DEADZONE && oldX >= -ANALOG_DEADZONE) {
			generateInput(InputLeft, ActionPressed);
		}
		if (oldX < -ANALOG_DEADZONE && newX >= -ANALOG_DEADZONE) {
			generateInput(InputLeft, ActionReleased);
		}
		if (newX > ANALOG_DEADZONE && oldX <= ANALOG_DEADZONE) {
			generateInput(InputRight, ActionPressed);
		}
		if (oldX > ANALOG_DEADZONE && newX <= ANALOG_DEADZONE) {
			generateInput(InputRight, ActionReleased);
		}
		if (newY < -ANALOG_DEADZONE && oldY >= -ANALOG_DEADZONE) {
			generateInput(InputUp, ActionPressed);
		}
		if (oldY < -ANALOG_DEADZONE && newY >= -ANALOG_DEADZONE) {
			generateInput(InputUp, ActionReleased);
		}
		if (newY > ANALOG_DEADZONE && oldY <= ANALOG_DEADZONE) {
			generateInput(InputDown, ActionPressed);
		}
		if (oldY > ANALOG_DEADZONE && newY <= ANALOG_DEADZONE) {
			generateInput(InputDown, ActionReleased);
		}
	}
}

void updatePoll(void)
{
	if (nextPollTime == -1)
		nextPollTime = getTime();

	// Get events from the system and immediately execute event callbacks
	glfwPollEvents();
	// Polling for gamepad events needs to be done manually
	pollGamepadEvents();

	// Handle direct quit events, like the [X] being clicked
	if (getState(PhaseMain) == StateRunning &&
	    glfwWindowShouldClose(window)) {
		setState(PhaseMain, StateUnstaged);
		logInfo("Exit signal received");
	}
}

void sleepPoll(void)
{
	nextPollTime += TIME_PER_POLL;
	sleep(nextPollTime);
}
