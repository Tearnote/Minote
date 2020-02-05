/**
 * Implementation of mapper.h
 * @file
 */

#include "mapper.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "window.h"
#include "queue.h"
#include "util.h"
#include "log.h"

/// Queue holding player inputs ready to be retrieved
static queue* inputs = null;

/// State of mapper system initialization
static bool initialized = false;

void mapperInit(void)
{
	if (initialized) return;
	inputs = queueCreate(sizeof(PlayerInput), 64);
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
	KeyInput key;
	while (windowInputDequeue(&key)) {
		queueEnqueue(inputs, &(PlayerInput){ .key = key.key, .action = key.action });
	}
}

bool mapperDequeue(PlayerInput* input)
{
	return queueDequeue(inputs, input);
}

bool mapperPeek(PlayerInput* input)
{
	return queuePeek(inputs, input);
}
