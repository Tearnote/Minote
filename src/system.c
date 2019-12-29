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
#include "util.h"

/// State of main system initialization
static bool initialized = false;

Log* syslog = null;

void systemInit(Log* log)
{
	assert(log);
	if (initialized) return;
	syslog = log;
	if (glfwInit() == GLFW_FALSE) {
		logCrit(syslog, u8"Failed to initialize GLFW: %s", systemError());
		syslog = null;
		exit(EXIT_FAILURE);
	}
	logDebug(syslog, u8"GLFW initialized");
	initialized = true;
}

void systemCleanup(void)
{
	if (!initialized) return;
	glfwTerminate();
	logDebug(syslog, u8"GLFW cleaned up");
	syslog = null;
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
