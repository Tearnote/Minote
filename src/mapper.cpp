/**
 * Implementation of mapper.h
 * @file
 */

#include "mapper.hpp"

#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "window.hpp"
#include "queue.hpp"
#include "util.hpp"
#include "log.hpp"

using minote::L;
using minote::queue;

/// Queue holding collectedInputs ready to be retrieved
static queue<Input, 64> inputs{};

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
 * Convert a GLFW action code to a ::Input state bool.
 * @param key Any GLFW action code
 * @return Matching bool value
 */
static bool actionToState(int action)
{
	assert(action == GLFW_PRESS || action == GLFW_RELEASE);
	if (action == GLFW_PRESS)
		return true;
	else
		return false;
}

void mapperInit(void)
{
	if (initialized) return;
	initialized = true;
}

void mapperCleanup(void)
{
	if (!initialized) return;
	initialized = false;
}

void mapperUpdate(void)
{
	assert(initialized);
	KeyInput key;
	while (windowInputDequeue(&key)) {
		InputType type = rawKeyToType(key.key);
		bool state = actionToState(key.action);
		if (type == InputNone)
			continue;
		Input newInput{
			.type = type,
			.state = state,
			.timestamp = key.timestamp
		};
		if(!inputs.enqueue(newInput))
			L.warn("Mapper queue full, input dropped");
	}
}

bool mapperDequeue(Input* input)
{
	assert(initialized);
	assert(input);
	Input* result = inputs.dequeue();
	if (result)
		*input = *result;
	return result;
}

bool mapperPeek(Input* input)
{
	assert(initialized);
	assert(input);
	Input* result = inputs.peek();
	if (result)
		*input = *result;
	return result;
}
