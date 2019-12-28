/**
 * Implementation of window.h
 * @file
 */

#include "window.h"

#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "util.h"
#include "queue.h"

struct Window {
	GLFWwindow* window; ///< Underlying GLFWwindow object
	const char* title; ///< Window title from the title bar
	Queue* inputs; ///< Message queue for storing keypresses
};

/// State of window system initialization
static bool initialized = false;

/// Log file used by the window system
static Log* winlog = null;

/**
 * Return the last GLFW error message and clear GLFW's error state
 * @return String literal describing the error. Use immediately, before calling
 * any other GLFW functions
 */
static const char* windowError(void)
{
	const char* description;
	int code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return u8"No error";
	return description;
}

/**
 * Function to run on each keypress event. The ::Window object is retrieved from
 * the GLFWwindow's user pointer.
 * @param window The GLFWwindow object
 * @param key Platform-independent key identifier
 * @param scancode Platform-dependent keycode
 * @param action GLFW_PRESS or GLFW_RELEASE
 * @param mods Bitmask of active modifier keys (ctrl, shift etc)
 */
static void
keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode, (void)mods;
	assert(glfwGetWindowUserPointer(window));
	if (action == GLFW_REPEAT) return; // Key repeat is not needed

	Window* w = glfwGetWindowUserPointer(window);
	if (!queueEnqueue(w->inputs, &(KeyInput){.key = key, .action = action}))
		logWarn(winlog, u8"Window input queue is full, key #%d %s dropped",
				key, action == GLFW_PRESS ? u8"press" : u8"release");
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
	Window* w = alloc(sizeof(*w));
	w->title = title;

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
		w->window = glfwCreateWindow(mode->width, mode->height, title,
				monitor, null);
	} else {
		w->window = glfwCreateWindow(size.x, size.y, title, null, null);
	}
	if (!w->window) {
		logCrit(winlog, u8"Failed to create window \"%s\": %s",
				title, windowError());
		exit(EXIT_FAILURE);
	}
	glfwSetWindowUserPointer(w->window, w);
	glfwSetInputMode(w->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	w->inputs = queueCreate(sizeof(KeyInput), 64);
	glfwSetKeyCallback(w->window, keyCallback);
	glfwGetFramebufferSize(w->window, &size.x, &size.y);
	logInfo(winlog, u8"Window \"%s\" created at %dx%d%s",
			title, size.x, size.y, fullscreen ? u8" fullscreen" : u8"");
	return w;
}

void windowDestroy(Window* w)
{
	glfwDestroyWindow(w->window);
	logDebug(winlog, u8"Window \"%s\" destroyed", w->title);
	free(w);
}

bool windowIsOpen(Window* w)
{
	return !glfwWindowShouldClose(w->window);
}
