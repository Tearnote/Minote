#include "window.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32
#include "base/math_io.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

namespace minote {

// The window with its OpenGL context active on current thread
static thread_local Window const* activeContext = nullptr;

// Retrieve the Window from raw GLFW handle by user pointer.
static auto getWindow(GLFWwindow* handle) -> Window&
{
	DASSERT(handle);
	return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(handle));
}

// Function to run on each keypress event. The event is added to the queue.
static void keyCallback(GLFWwindow* const handle, int const key, int, int const state, int)
{
	DASSERT(handle);
	if (state == GLFW_REPEAT) return; // Key repeat is not used
	auto& window = getWindow(handle);

	Window::KeyInput const input = {
		.key = key,
		.state = state,
		.timestamp = Glfw::getTime()
	};

	lock_guard const lock(window.inputsMutex);
	try {
		window.inputs.push_back(input);
	} catch (...) {
		L.warn(R"(Window "{}" input queue is full, key #{} {} dropped)",
			window.title, key, state == GLFW_PRESS? "press" : "release");
	}
}

// Function to run when the window is resized. The new size is kept for later
// retrieval, such as by Frame::begin().
static void framebufferResizeCallback(GLFWwindow* const handle, int const width, int const height)
{
	DASSERT(handle);
	DASSERT(width);
	DASSERT(height);
	auto& window = getWindow(handle);

	uvec2 const newSize = {width, height};
	window.size = newSize;
	L.info(R"(Window "{}" resized to {})", window.title, newSize);
}

// Function to run when the window is rescaled. This might happen when dragging
// it to a display with different DPI scaling, or at startup. The new scale
// is saved for later retrieval.
static void windowScaleCallback(GLFWwindow* const handle, float const xScale, float)
{
	DASSERT(handle);
	DASSERT(xScale);
	// yScale seems to sometimes be 0.0, so it is not reliable
	auto& window = getWindow(handle);

	window.scale = xScale;
	L.info(R"(Window "{}" DPI scaling changed to {})", window.title, xScale);
}

Window::Window(Glfw const&, string_view _title, bool const fullscreen, uvec2 _size)
{
	DASSERT(_size.x > 0 && _size.y > 0);

	// *** Set up context params ***

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
	glfwWindowHint(GLFW_STENCIL_BITS, 0); // As above
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // Declare DPI awareness

	// *** Create the window handle ***

	title = _title;
	auto* monitor = glfwGetPrimaryMonitor();
	if (!monitor)
		L.fail("Failed to query primary monitor: {}", Glfw::getError());
	auto const* const mode = glfwGetVideoMode(monitor);
	if (!mode)
		L.fail("Failed to query video mode: {}", Glfw::getError());
	if (fullscreen)
		_size = {mode->width, mode->height};
	else
		monitor = nullptr;

	handle = glfwCreateWindow(_size.x, _size.y, title.c_str(), monitor, nullptr);
	if (!handle)
		L.fail(R"(Failed to create window "{}": {})", _title, Glfw::getError());

	// *** Set window properties ***

	isContextActive = false;

	// Real size might be different from requested size because of DPI scaling
	ivec2 realSize;
	glfwGetFramebufferSize(handle, &realSize.x, &realSize.y);
	if  (realSize.x == 0 || realSize.y == 0)
		L.fail(R"(Failed to retrieve window "{}"'s framebuffer size: {})", _title, Glfw::getError());
	size = realSize;

	float _scale;
	glfwGetWindowContentScale(handle, &_scale, nullptr);
	scale = _scale;

	// *** Set up window callbacks ***

	glfwSetWindowUserPointer(handle, this);
#ifndef MINOTE_DEBUG
	glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif //MINOTE_DEBUG
	glfwSetKeyCallback(handle, keyCallback);
	glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(handle, windowScaleCallback);

	L.info(R"(Window "{}" created at {} *{}{})",
		title, size.load(), _scale, fullscreen ? " fullscreen" : "");
}

Window::~Window()
{
	DASSERT(!isContextActive);

	glfwDestroyWindow(handle);

	L.info(R"(Window "{}" closed)", title);
}

auto Window::isClosing() const -> bool
{
	lock_guard const lock(handleMutex);
	return glfwWindowShouldClose(handle);
}

void Window::requestClose()
{
	lock_guard const lock(handleMutex);
	glfwSetWindowShouldClose(handle, true);

	L.info(R"(Window "{}" close requested)", title);
}

void Window::flip()
{
	glfwSwapBuffers(handle);
}

void Window::activateContext()
{
	DASSERT(!isContextActive);
	DASSERT(!activeContext);

	glfwMakeContextCurrent(handle);
	isContextActive = true;
	activeContext = this;

	L.debug(R"(Window "{}" OpenGL context activated)", title);
}

void Window::deactivateContext()
{
	DASSERT(isContextActive);
	DASSERT(activeContext == this);

	glfwMakeContextCurrent(nullptr);
	isContextActive = false;
	activeContext = nullptr;

	L.debug(R"(Window "{}" OpenGL context deactivated)", title);
}

auto Window::dequeueInput() -> optional<KeyInput>
{
	lock_guard const lock(inputsMutex);
	if (inputs.empty()) return nullopt;

	KeyInput input{inputs.front()};
	inputs.pop_front();
	return input;
}

auto Window::peekInput() const -> optional<KeyInput>
{
	lock_guard const lock(inputsMutex);
	if (inputs.empty()) return nullopt;

	return inputs.front();
}

void Window::clearInput()
{
	lock_guard const lock(inputsMutex);

	inputs.clear();
}

}
