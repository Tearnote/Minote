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
	assert(initialized);
	KeyInput key;
	while (windowInputDequeue(&key)) {
		queueEnqueue(inputs, &(PlayerInput){ .key = key.key, .action = key.action, .timestamp = key.timestamp });
	}
}

bool mapperDequeue(PlayerInput* input)
{
	assert(initialized);
	assert(input);
	return queueDequeue(inputs, input);
}

bool mapperPeek(PlayerInput* input)
{
	assert(initialized);
	assert(input);
	return queuePeek(inputs, input);
}
