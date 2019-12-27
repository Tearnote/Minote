#include "window.h"

#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "util.h"

struct Window {
	GLFWwindow* window;
	const char* title;
};

static bool initialized = false;

static Log* winlog = null;

static const char* windowError(void)
{
	const char* description;
	int code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return u8"No error";
	return description;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode, (void)mods;
	assert(glfwGetWindowUserPointer(window));

	Window* object = glfwGetWindowUserPointer(window);
	if (action == GLFW_PRESS)
		logTrace(winlog, u8"Window %s received keypress %d", object->title, key);
	//TOTO push an input onto the stack
}

void windowInit(Log* log)
{
	assert(log);
	if (initialized) return;
	winlog = log;
	if (glfwInit() == GLFW_FALSE) {
		logCrit(winlog, u8"Failed to initialize GLFW: %s", windowError());
		winlog = null;
		exit(EXIT_FAILURE);
	}
	logDebug(winlog, u8"GLFW initialized");
	initialized = true;
}

void windowCleanup(void)
{
	if (!initialized) return;
	glfwTerminate();
	logDebug(winlog, u8"GLFW cleaned up");
	winlog = null;
	initialized = false;
}

void windowPoll(void)
{
	glfwPollEvents();
}

Window* windowCreate(const char* title, Size2i size, bool fullscreen)
{
	assert(title);
	assert(size.x >= 0 && size.y >= 0);
	Window* result = alloc(sizeof(*result));
	result->title = title;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // __APPLE__
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // DPI aware
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE); // Linear gamma
	glfwWindowHint(GLFW_SAMPLES, 4); // 4xMSAA
	if (fullscreen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		result->window = glfwCreateWindow(mode->width, mode->height, title,
				monitor, null);
	} else {
		result->window = glfwCreateWindow(size.x, size.y, title, null, null);
	}
	if (!result->window) {
		logCrit(winlog, u8"Failed to create window %s: %s", title, windowError());
		exit(EXIT_FAILURE);
	}
	glfwSetWindowUserPointer(result->window, result);
	glfwSetInputMode(result->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetKeyCallback(result->window, keyCallback);
	glfwGetFramebufferSize(result->window, &size.x, &size.y);
	logInfo(winlog, u8"Created a %s%dx%d window",
			fullscreen ? u8"fullscreen " : u8"", size.x, size.y);
	return result;
}

void windowDestroy(Window* w)
{
	glfwDestroyWindow(w->window);
	logDebug(winlog, u8"Window %s destroyed", w->title);
	free(w);
}

bool windowIsOpen(Window* w)
{
	return !glfwWindowShouldClose(w->window);
}