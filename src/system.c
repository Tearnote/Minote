/**
 * Implementation of system.h
 * @file
 */

#include "system.h"

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#endif //_WIN32
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
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR) {
		logCrit(applog, u8"Failed to initialize Windows timer");
		exit(EXIT_FAILURE);
	}
#endif //_WIN32
	logDebug(applog, u8"GLFW initialized");
	initialized = true;
}

void systemCleanup(void)
{
	if (!initialized) return;
#ifdef _WIN32
	timeEndPeriod(1);
#endif //_WIN32
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
