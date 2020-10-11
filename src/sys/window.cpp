/**
 * Implementation of window.h
 * @file
 */

#include "window.hpp"

#include <cstdlib>
#include <atomic>
#include <mutex>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32
#include <GLFW/glfw3.h>
#include "base/queue.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "debug.hpp"

using namespace minote;

static thread_local Window* activeContext = nullptr;

/**
 * Retrieve the GLFW description of the most recently encountered error,
 * and clear the GLFW error state.
 * @return Error description string literal. Use ASAP, before the next GLFW call
 */
static auto glfwError() -> const char*
{
	const char* description = nullptr;
	const int code = glfwGetError(&description);
	if (code == GLFW_NO_ERROR)
		return "No error";
	ASSERT(description);
	return description;
}

/**
 * Trivial function to retrieve the Window from raw GLFW handle by user pointer.
 * @param handle Raw GLFW window
 * @return Window previously provided to the handle by glfwSetWindowUserPointer()
 */
static auto getWindow(GLFWwindow* handle) -> Window&
{
	ASSERT(handle);
	return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(handle));
}

/**
 * Function to run on each keypress event. The key event is added to the queue.
 * @param handle Raw GLFW window
 * @param key Platform-independent key identifier
 * @param scancode Platform-dependent keycode
 * @param state GLFW_PRESS or GLFW_RELEASE
 * @param mods Bitmask of active modifier keys (ctrl, shift etc.)
 */
static void keyCallback(GLFWwindow* handle, int key, int, int state, int)
{
	ASSERT(handle);
	if (state == GLFW_REPEAT)
		return; // Key repeat is not used
	auto& window = getWindow(handle);

	Window::KeyInput input = {
		.key = key,
		.state = state,
		.timestamp = Window::getTime()
	};

	window.inputsMutex.lock();
	const auto success = window.inputs.enqueue(input);
	window.inputsMutex.unlock();
	if (!success)
		L.warn(R"(Window "%s" input queue is full, key #%d %s dropped)",
			window.title, key, state == GLFW_PRESS ? "press" : "release");
}

/**
 * Function to run when the window is resized. The new size is kept for later
 * retrieval, most likely by the renderer.
 * @param handle Raw GLFW window
 * @param width New window width in pixels
 * @param height New window height in pixels
 */
static void framebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
	ASSERT(handle);
	ASSERT(width);
	ASSERT(height);
	auto& window = getWindow(handle);

	const size2i newSize = {width, height};
	window.size = newSize;
	L.info(R"(Window "%s" resized to %dx%d)", window.title, width, height);
}

/**
 * Function to run when the window is rescaled. This might happen when dragging
 * it to a display with different DPI scaling, or at startup. The new scale
 * is saved for later retrieval.
 * @param handle Raw GLFW window
 * @param xScale New window scale, with 1.0 being "normal"
 * @param yScale Unused. This appears to be 0.0 sometimes so we ignore it
 */
static void windowScaleCallback(GLFWwindow* handle, float xScale, float)
{
	ASSERT(handle);
	ASSERT(xScale);
	auto& window = getWindow(handle);

	window.scale = xScale;
	L.info(R"(Window "%s" DPI scaling changed to %f)", window.title, xScale);
}

namespace minote {

void Window::init()
{
	ASSERT(!initialized);

	if (glfwInit() == GLFW_FALSE)
		L.fail("Failed to initialize GLFW: %s", glfwError());
#ifdef _WIN32
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		L.fail("Failed to initialize Windows timer");
#endif //_WIN32

	L.info("Windowing initialized");
	initialized = true;
}

void Window::cleanup()
{
#ifndef NDEBUG
	if (!initialized) {
		L.warn("Windowing cleanup called without being previously initialized");
		return;
	}
#endif //NDEBUG

#ifdef _WIN32
	timeEndPeriod(1);
#endif //_WIN32
	glfwTerminate();

	L.debug("Windowing cleaned up");
	initialized = false;
}

void Window::poll()
{
	glfwPollEvents();
}

auto Window::getTime() -> nsec
{
	return seconds(glfwGetTime());
}

void Window::open(char const* const _title, const bool fullscreen, size2i _size)
{
	ASSERT(_title);
	ASSERT(_size.x >= 0 && _size.y >= 0);
	ASSERT(initialized);

	// Set up context params
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
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // Declare DPI awareness

	// Create the window handle
	const auto[wantedSize, monitor] = [=]() -> std::pair<size2i, GLFWmonitor*> {
		if (fullscreen) {
			GLFWmonitor* const monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* const mode = glfwGetVideoMode(monitor);
			return {{mode->width, mode->height}, monitor};
		} else {
			return {_size, nullptr};
		}
	}();
	handle = glfwCreateWindow(wantedSize.x, wantedSize.y, _title, monitor, nullptr);
	if (!handle)
		L.fail(R"(Failed to create window "%s": %s)", title, glfwError());

	// Get window properties
	const size2i realSize = [=, this] { // Might be different from wanted size because of DPI scaling
		size2i s;
		glfwGetFramebufferSize(handle, &s.x, &s.y);
		return s;
	}();
	size = realSize;
	const float _scale = [=, this] {
		float s = 0.0f;
		glfwGetWindowContentScale(handle, &s, nullptr);
		return s;
	}();
	scale = _scale;
	title = _title;

	// Set up window callbacks
	glfwSetWindowUserPointer(handle, this);
#ifndef MINOTE_DEBUG
	glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif //MINOTE_DEBUG
	glfwSetKeyCallback(handle, keyCallback);
	glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(handle, windowScaleCallback);

	L.info(R"(Window "%s" created at %dx%d *%f%s)",
		stringOrNull(_title), realSize.x, realSize.y, _scale, fullscreen ? " fullscreen" : "");
}

void Window::close()
{
#ifndef NDEBUG
	if (!initialized) {
		L.warn(R"(Tried to close window "%s" without initialized windowing)",
			stringOrNull(title));
		return;
	}
	if (!handle) {
		L.warn(R"(Tried to close window "%s" which is not open)",
			stringOrNull(title));
		return;
	}
#endif //NDEBUG

	glfwDestroyWindow(handle);
	handle = nullptr;

	L.info(R"(Window "%s" closed)", stringOrNull(title));
}

auto Window::isClosing() const -> bool
{
	ASSERT(initialized);
	ASSERT(handle);

	std::lock_guard lock(handleMutex);
	return glfwWindowShouldClose(handle);
}

void Window::requestClose()
{
	ASSERT(initialized);
	ASSERT(handle);

	handleMutex.lock();
	glfwSetWindowShouldClose(handle, true);
	handleMutex.unlock();
	L.info(R"(Window "%s" close requested)", stringOrNull(title));
}

void Window::flip()
{
	ASSERT(initialized);
	ASSERT(handle);

	glfwSwapBuffers(handle);
}

void Window::activateContext()
{
	ASSERT(initialized);
	ASSERT(handle);
	ASSERT(!isContextActive);
	ASSERT(!activeContext);

	glfwMakeContextCurrent(handle);
	isContextActive = true;
	activeContext = this;
	L.debug(R"(Window "%s" OpenGL context activated)", stringOrNull(title));
}

void Window::deactivateContext()
{
#ifndef NDEBUG
	if (!initialized) {
		L.warn(R"(Tried to deactivate context of window "%s" without initialized windowing)",
			stringOrNull(title));
		return;
	}
	if (!handle) {
		L.warn(R"(Tried to deactivate context of window "%s" which is not open)",
			stringOrNull(title));
		return;
	}
	if (!isContextActive) {
		L.warn(R"(Tried to deactivate context of window "%s" without an active context)",
			stringOrNull(title));
		return;
	}
	if (activeContext != this) {
		L.warn(R"(Tried to deactivate context of window "%s" on the wrong thread)",
			stringOrNull(title));
		return;
	}
#endif //NDEBUG

	glfwMakeContextCurrent(nullptr);
	isContextActive = false;
	activeContext = nullptr;
	L.debug(R"(Window "%s" OpenGL context deactivated)", stringOrNull(title));
}

auto Window::dequeueInput() -> std::optional<KeyInput>
{
	const std::lock_guard lock(inputsMutex);

	const auto* input = inputs.dequeue();
	if (input)
		return *input;
	else
		return std::nullopt;
}

auto Window::peekInput() const -> std::optional<KeyInput>
{
	const std::lock_guard lock(inputsMutex);

	const auto* input = inputs.peek();
	if (input)
		return *input;
	else
		return std::nullopt;
}

void Window::clearInput()
{
	inputsMutex.lock();
	inputs.clear();
	inputsMutex.unlock();
}

Window::~Window()
{
#ifndef NDEBUG
	if(handle)
		L.warn(R"(Window "%s" was never closed)", stringOrNull(title));
#endif //NDEBUG
}

}
