/**
 * Implementation of time.h
 * @file
 */

#include "time.h"

#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#else //_WIN32
#include <string.h>
#include <errno.h>
#include <time.h>
#include "util.h"
#include "log.h"
#endif //_WIN32

nsec getTime(void)
{
	return secToNsec(glfwGetTime());
}

void sleepFor(nsec duration)
{
	assert(duration > 0);
#ifdef _WIN32
	Sleep(duration / (secToNsec(1) / 1000));
#else //_WIN32
	struct timespec spec;
	spec.tv_nsec = (long)(duration % secToNsec(1));
	spec.tv_sec = (long)(duration / secToNsec(1));
	if (nanosleep(&spec, null) == -1)
		logDebug(applog, "Sleep was interrupted: %s", strerror(errno));
#endif //_WIN32
}
