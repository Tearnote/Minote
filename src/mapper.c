/**
 * Implementation of mapper.h
 * @file
 */

#include "mapper.h"

#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "window.h"
#include "queue.h"
#include "util.h"

/// Queue holding inputs ready to be retrieved
static queue* inputs = null;

static bool initialized = false;

/**
 * Convert a GLFW key code to a ::InputType.
 * @param key Any GLFW key code
 * @return Matching ::InputType value, or ::InputNone
 */
static InputType rawKeyToType(int key)
{
	switch (key) {
	case GLFW_KEY_UP:
	case GLFW_KEY_W:
		return InputUp;
	case GLFW_KEY_DOWN:
	case GLFW_KEY_S:
		return InputDown;
	case GLFW_KEY_LEFT:
	case GLFW_KEY_A:
		return InputLeft;
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_D:
		return InputRight;
	case GLFW_KEY_Z:
	case GLFW_KEY_J:
		return InputButton1;
	case GLFW_KEY_X:
	case GLFW_KEY_K:
		return InputButton2;
	case GLFW_KEY_C:
	case GLFW_KEY_L:
		return InputButton3;
	case GLFW_KEY_SPACE:
		return InputButton4;
	case GLFW_KEY_ENTER:
		return InputStart;
	case GLFW_KEY_ESCAPE:
		return InputQuit;
	default:
		return InputNone;
	}
}

/**
 * Convert a GLFW action code to a ::InputAction.
 * @param key Any GLFW action code
 * @return Matching ::InputAction value, or ::ActionNone
 */
static InputAction rawActionToAction(int action)
{
	switch (action) {
	case GLFW_PRESS:
		return ActionPressed;
	case GLFW_RELEASE:
		return ActionReleased;
	default:
		return ActionNone;
	}
}

void mapperInit(void)
{
	if (initialized) return;
	inputs = queueCreate(sizeof(GameInput), 64);
	initialized = true;
}

void mapperCleanup(void)
{
	if (!initialized) return;
	if (inputs) {
		queueDestroy(inputs);
		inputs = null;
	}
	initialized = false;
}

void mapperUpdate(void)
{
	assert(initialized);
	KeyInput key;
	while (windowInputDequeue(&key)) {
		InputType type = rawKeyToType(key.key);
		InputAction action = rawActionToAction(key.action);
		if (type == InputNone || action == ActionNone)
			continue;
		queueEnqueue(inputs, &(GameInput){
			.type = type,
			.action = action,
			.timestamp = key.timestamp});
	}
}

bool mapperDequeue(GameInput* input)
{
	assert(initialized);
	assert(input);
	return queueDequeue(inputs, input);
}

bool mapperPeek(GameInput* input)
{
	assert(initialized);
	assert(input);
	return queuePeek(inputs, input);
}
