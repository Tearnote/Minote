/**
 * Implementation of window.h
 * @file
 */

#include "window.hpp"

#include <atomic>
#include <mutex>
#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "system.hpp"
#include "queue.hpp"
#include "debug.hpp"
#include "util.hpp"
#include "log.hpp"

using minote::log::L;

static bool initialized = false;
static GLFWwindow* window; ///< Underlying GLFWwindow object
static const char* windowTitle; ///< Window title from the title bar
static queue* inputs; ///< Message queue for storing keypresses
static std::mutex inputsMutex; ///< Mutex protecting the #collectedInputs queue
static std::atomic<bool> windowOpen; ///< false if window should be closed, true otherwise
// These two are not #size2i because the struct cannot be atomic
static std::atomic<size_t> viewportWidth; ///< in pixels
static std::atomic<size_t> viewportHeight; ///< in pixels
static std::atomic<float> viewportScale; ///< DPI scaling of the window, where 1.0 is "normal"
/**
 * Function to run on each keypress event. The key event is added to the queue.
 * @param w Unused
 * @param key Platform-independent key identifier
 * @param scancode Platform-dependent keycode
 * @param action GLFW_PRESS or GLFW_RELEASE
 * @param mods Bitmask of active modifier keys (ctrl, shift etc)
 */
static void
keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
{
	(void)w, (void)scancode, (void)mods;
	assert(initialized);
	if (action == GLFW_REPEAT) return; // Key repeat is not needed
	KeyInput input = {.key = key, .action = action, .timestamp = getTime()};
	inputsMutex.lock();
	if (!queueEnqueue(inputs, &input))
		L.warn("Window input queue is full, key #%d %s dropped",
			key, action == GLFW_PRESS ? "press" : "release");
	inputsMutex.unlock();
}

/**
 * Function to run when user requested window close.
 * @param w Unused
 */
static void windowCloseCallback(GLFWwindow* w)
{
	(void)w;
	assert(initialized);
	windowOpen = false;
}

/**
 * Function to run when the window is resized. The new size is kept for later
 * retrieval with windowGetSize().
 * @param w Unused
 * @param width New window width in pixels
 * @param height New window height in pixels
 */
static void framebufferResizeCallback(GLFWwindow* w, int width, int height)
{
	(void)w;
	assert(initialized);
	assert(width);
	assert(height);
	viewportWidth = width;
	viewportHeight = height;
	L.debug("Window \"%s\" resized to %dx%d",
		windowTitle, width, height);
}

/**
 * Function to run when the window is rescaled. This might happen when dragging
 * it to a display with different DPI scaling, or at startup. The new scale is
 * kept for later retrieval with windowGetScale().
 * @param w Unused
 * @param xScale New window scale, with 1.0 being "normal"
 * @param yScale Unused. This appears to be 0.0 sometimes so we ignore it
 */
static void windowScaleCallback(GLFWwindow* w, float xScale, float yScale)
{
	(void)w, (void)yScale;
	assert(initialized);
	assert(xScale);
	viewportScale = xScale;
	L.debug("Window \"%s\" DPI scaling changed to %f",
		windowTitle, xScale);
}

void windowInit(const char* title, size2i size, bool fullscreen)
{
	assert(title);
	assert(size.x >= 0 && size.y >= 0);
	if (initialized) return;
	windowTitle = title;
	windowOpen = true;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // __APPLE__
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 0);
	glfwWindowHint(GLFW_DEPTH_BITS, 0); // Handled by an internal FB
	glfwWindowHint(GLFW_STENCIL_BITS, 0); // Same
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // DPI aware
	if (fullscreen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		window = glfwCreateWindow(mode->width, mode->height, title, monitor,
			nullptr);
	} else {
		window = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
	}
	if (!window) {
		L.crit("Failed to create window \"%s\": %s", title,
			systemError());
		exit(EXIT_FAILURE);
	}
	initialized = true;

#ifndef MINOTE_DEBUG
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif //MINOTE_DEBUG
	inputs = queueCreate(sizeof(KeyInput), 64);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetWindowCloseCallback(window, windowCloseCallback);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(window, windowScaleCallback);
	// An initial check is required to get correct values for non-100% scaling
	int width = 0;
	int height = 0;
	float scale = 0.0f;
	glfwGetFramebufferSize(window, &width, &height);
	framebufferResizeCallback(window, width, height);
	glfwGetWindowContentScale(window, &scale, nullptr);
	windowScaleCallback(window, scale, 0);
	L.info("Window \"%s\" created at %dx%d *%f%s",
		title, width, height, scale, fullscreen ? " fullscreen" : "");
}

void windowCleanup(void)
{
	if (!initialized) return;
	queueDestroy(inputs);
	inputs = nullptr;
	glfwDestroyWindow(window);
	window = nullptr;
	L.debug("Window \"%s\" destroyed", windowTitle);
	windowTitle = nullptr;
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
	return windowOpen;
}

void windowClose(void)
{
	assert(initialized);
	windowOpen = false;
}

const char* windowGetTitle(void)
{
	assert(initialized);
	return windowTitle;
}

size2i windowGetSize(void)
{
	assert(initialized);
	return (size2i){viewportWidth, viewportHeight};
}

float windowGetScale(void)
{
	assert(initialized);
	return viewportScale;
}

void windowContextActivate(void)
{
	assert(initialized);
	glfwMakeContextCurrent(window);
}

void windowContextDeactivate(void)
{
	assert(initialized);
	glfwMakeContextCurrent(nullptr);
}

void windowFlip(void)
{
	assert(initialized);
	glfwSwapBuffers(window);
}

bool windowInputDequeue(KeyInput* input)
{
	assert(initialized);
	inputsMutex.lock();
	bool result = queueDequeue(inputs, input);
	inputsMutex.unlock();
	return result;
}

bool windowInputPeek(KeyInput* input)
{
	assert(initialized);
	inputsMutex.lock();
	bool result = queuePeek(inputs, input);
	inputsMutex.unlock();
	return result;
}

void windowInputClear(void)
{
	assert(initialized);
	inputsMutex.lock();
	queueClear(inputs);
	inputsMutex.unlock();
}

GLFWwindow* getRawWindow(void)
{
	assert(window);
	return window;
}
