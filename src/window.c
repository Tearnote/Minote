/**
 * Implementation of window.h
 * @file
 */

#include "window.h"

#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "thread.h"
#include "system.h"
#include "queue.h"
#include "util.h"
#include "log.h"

typedef struct Window {
	GLFWwindow* window; ///< Underlying GLFWwindow object
	const char* title; ///< Window title from the title bar
	Queue* inputs; ///< Message queue for storing keypresses
	mutex* inputsMutex; ///< Mutex protecting the #inputs queue
	atomic bool open; ///< false if window should be closed, true otherwise
	// These two are not #Size2i because the struct cannot be atomic
	atomic size_t width; ///< width of the viewport in pixels
	atomic size_t height; ///< height of the viewport in pixels
	atomic float scale; ///< DPI scaling of the window, where 1.0 is "normal"
} Window;

/// State of window system initialization
static bool initialized = false;

/// Global window instance
static Window appwindow = {0};

/**
 * Function to run on each keypress event. The key event is added to the queue.
 * @param window Unused
 * @param key Platform-independent key identifier
 * @param scancode Platform-dependent keycode
 * @param action GLFW_PRESS or GLFW_RELEASE
 * @param mods Bitmask of active modifier keys (ctrl, shift etc)
 */
static void
keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)window, (void)scancode, (void)mods;
	assert(initialized);
	if (action == GLFW_REPEAT) return; // Key repeat is not needed
	mutexLock(appwindow.inputsMutex);
	if (!queueEnqueue(appwindow.inputs,
		&(KeyInput){.key = key, .action = action}))
		logWarn(applog, u8"Window input queue is full, key #%d %s dropped",
			key, action == GLFW_PRESS ? u8"press" : u8"release");
	mutexUnlock(appwindow.inputsMutex);
}

/**
 * Function to run when user requested window close.
 * @param window Unused
 */
static void windowCloseCallback(GLFWwindow* window)
{
	(void)window;
	assert(initialized);
	appwindow.open = false;
}

/**
 * Function to run when the window is resized. The new size is kept for later
 * retrieval with windowGetSize().
 * @param window Unused
 * @param width New window width in pixels
 * @param height New window height in pixels
 */
static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	(void)window;
	assert(initialized);
	assert(width);
	assert(height);
	appwindow.width = width;
	appwindow.height = height;
	logDebug(applog, u8"Window \"%s\" resized to %dx%d",
		appwindow.title, width, height);
}

/**
 * Function to run when the window is rescaled. This might happen when dragging
 * it to a display with different DPI scaling, or at startup. The new scale is
 * kept for later retrieval with windowGetScale().
 * @param window Unused
 * @param xScale New window scale, with 1.0 being "normal"
 * @param yScale Unused. This appears to be 0.0 sometimes so we ignore it
 */
static void windowScaleCallback(GLFWwindow* window, float xScale, float yScale)
{
	(void)window, (void)yScale;
	assert(initialized);
	assert(xScale);
	appwindow.scale = xScale;
	logDebug(applog, "Window \"%s\" DPI scaling changed to %f",
		appwindow.title, xScale);
}

void windowInit(const char* title, Size2i size, bool fullscreen)
{
	assert(title);
	assert(size.x >= 0 && size.y >= 0);
	if (initialized) return;
	appwindow.title = title;
	atomic_init(&appwindow.open, true);
	appwindow.inputsMutex = mutexCreate();
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
		appwindow.window = glfwCreateWindow(mode->width, mode->height, title,
			monitor, null);
	} else {
		appwindow.window = glfwCreateWindow(size.x, size.y, title, null, null);
	}
	if (!appwindow.window) {
		logCrit(applog, u8"Failed to create window \"%s\": %s",
			title, systemError());
		exit(EXIT_FAILURE);
	}
	initialized = true;

	glfwSetInputMode(appwindow.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	appwindow.inputs = queueCreate(sizeof(KeyInput), 64);
	glfwSetKeyCallback(appwindow.window, keyCallback);
	glfwSetWindowCloseCallback(appwindow.window, windowCloseCallback);
	glfwSetFramebufferSizeCallback(appwindow.window, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(appwindow.window, windowScaleCallback);
	// An initial check is required to get correct values for non-100% scaling
	int width = 0;
	int height = 0;
	float scale = 0.0f;
	glfwGetFramebufferSize(appwindow.window, &width, &height);
	framebufferResizeCallback(appwindow.window, width, height);
	glfwGetWindowContentScale(appwindow.window, &scale, null);
	windowScaleCallback(appwindow.window, scale, 0);
	logInfo(applog, u8"Window \"%s\" created at %dx%d*%f%s",
		title, width, height, scale, fullscreen ? u8" fullscreen" : u8"");
}

void windowCleanup(void)
{
	if (!initialized) return;
	queueDestroy(appwindow.inputs);
	appwindow.inputs = null;
	glfwDestroyWindow(appwindow.window);
	appwindow.window = null;
	mutexDestroy(appwindow.inputsMutex);
	appwindow.inputsMutex = null;
	logDebug(applog, u8"Window \"%s\" destroyed", appwindow.title);
	initialized = false;
}

void windowPoll(void)
{
	assert(initialized);
	glfwPollEvents();
}

bool windowIsOpen(void)
{
	assert(initialized);
	return appwindow.open;
}

void windowClose(void)
{
	assert(initialized);
	appwindow.open = false;
}

const char* windowGetTitle(void)
{
	assert(initialized);
	return appwindow.title;
}

Size2i windowGetSize(void)
{
	assert(initialized);
	return (Size2i){appwindow.width, appwindow.height};
}

float windowGetScale(void)
{
	assert(initialized);
	return appwindow.scale;
}

void windowContextActivate(void)
{
	assert(initialized);
	glfwMakeContextCurrent(appwindow.window);
}

void windowContextDeactivate(void)
{
	assert(initialized);
	glfwMakeContextCurrent(null);
}

void windowFlip(void)
{
	assert(initialized);
	glfwSwapBuffers(appwindow.window);
}

bool windowInputDequeue(KeyInput* input)
{
	assert(initialized);
	mutexLock(appwindow.inputsMutex);
	bool result = queueDequeue(appwindow.inputs, input);
	mutexUnlock(appwindow.inputsMutex);
	return result;
}

bool windowInputPeek(KeyInput* input)
{
	assert(initialized);
	mutexLock(appwindow.inputsMutex);
	bool result = queuePeek(appwindow.inputs, input);
	mutexUnlock(appwindow.inputsMutex);
	return result;
}

void windowInputClear(void)
{
	assert(initialized);
	mutexLock(appwindow.inputsMutex);
	queueClear(appwindow.inputs);
	mutexUnlock(appwindow.inputsMutex);
}
