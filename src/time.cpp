/**
 * Implementation of time.h
 * @file
 */

#include "time.hpp"

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#else //_WIN32
#include <string.h>
#include <errno.h>
#include <time.h>
#include "base/log.hpp"
#endif //_WIN32
#include "base/util.hpp"

nsec getTime(void)
{
	return secToNsec(glfwGetTime());
}

void sleepFor(nsec duration)
{
	ASSERT(duration > 0);
#ifdef _WIN32
	Sleep(duration / (secToNsec(1) / 1000));
#else //_WIN32
	struct timespec spec;
	spec.tv_nsec = (long)(duration % secToNsec(1));
	spec.tv_sec = (long)(duration / secToNsec(1));
	if (nanosleep(&spec, null) == -1)
		L.debug("Sleep was interrupted: %s", strerror(errno));
#endif //_WIN32
}
