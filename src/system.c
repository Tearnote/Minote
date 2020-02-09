/**
 * Implementation of system.h
 * @file
 */

#include "system.h"

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "log.h"

static bool initialized = false;

void systemInit(void)
{
	if (initialized) return;
	assert(applog);
	if (glfwInit() == GLFW_FALSE) {
		logCrit(applog, u8"Failed to initialize GLFW: %s", systemError());
		exit(EXIT_FAILURE);
	}
	logDebug(applog, u8"GLFW initialized");
	initialized = true;
}

void systemCleanup(void)
{
	if (!initialized) return;
	glfwTerminate();
	if (applog) logDebug(applog, u8"GLFW cleaned up");
	initialized = false;
}

const char* systemError(void)
{
	const char* description;
	int code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return u8"No error";
	return description;
}
