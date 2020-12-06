#include "window.hpp"

#include <GLFW/glfw3.h>
#include "base/math_io.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

namespace minote {

// The window with its OpenGL context active on current thread
static thread_local Window const* activeContext{nullptr};

// Retrieve the Window from raw GLFW handle by user pointer.
static auto getWindow(GLFWwindow* handle) -> Window& {
	DASSERT(handle);
	return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(handle));
}

void Window::keyCallback(GLFWwindow* const handle,
	int const rawKeycode, int const rawScancode, int const rawState, int) {
	DASSERT(handle);
	if (rawState == GLFW_REPEAT) return; // Key repeat is not used
	auto& window{getWindow(handle)};
	using State = KeyInput::State;

	Keycode const keycode{rawKeycode};
	Scancode const scancode{rawScancode};
	auto const name{window.glfw.getKeyName(keycode, scancode)};
	State const state{rawState == GLFW_PRESS? State::Pressed : State::Released};
	KeyInput const input{
		.keycode = keycode,
		.scancode = scancode,
		.name = name,
		.state = state,
		.timestamp = Glfw::getTime()
	};

	lock_guard const lock{window.inputsMutex};
	try {
		window.inputs.push_back(input);
	} catch (...) {
		L.warn(R"(Window "{}" input queue full, key "{}" {} event dropped)",
			window.title(), name, state == State::Pressed? "press" : "release");
	}
}

void Window::framebufferResizeCallback(GLFWwindow* const handle, int const width, int const height) {
	DASSERT(handle);
	DASSERT(width);
	DASSERT(height);
	auto& window{getWindow(handle)};

	uvec2 const newSize{width, height};
	window.m_size = newSize;
	L.info(R"(Window "{}" resized to {})", window.title(), newSize);
}

// Function to run when the window is rescaled. This might happen when dragging
// it to a display with different DPI scaling, or at startup. The new scale
// is saved for later retrieval.
void Window::windowScaleCallback(GLFWwindow* const handle, float const xScale, float) {
	DASSERT(handle);
	DASSERT(xScale);
	// yScale seems to sometimes be 0.0, so it is not reliable
	auto& window{getWindow(handle)};

	window.m_scale = xScale;
	L.info(R"(Window "{}" DPI scaling changed to {})", window.title(), xScale);
}

Window::Window(Glfw const& _glfw, string_view _title, bool const fullscreen, uvec2 _size) noexcept:
	glfw{_glfw}, m_title{_title}, isContextActive{false} {
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

	auto* monitor{glfwGetPrimaryMonitor()};
	if (!monitor)
		L.fail("Failed to query primary monitor: {}", Glfw::getError());
	auto const* const mode{glfwGetVideoMode(monitor)};
	if (!mode)
		L.fail("Failed to query video mode: {}", Glfw::getError());
	if (fullscreen)
		_size = {mode->width, mode->height};
	else
		monitor = nullptr;

	handle = glfwCreateWindow(_size.x, _size.y, m_title.c_str(), monitor, nullptr);
	if (!handle)
		L.fail(R"(Failed to create window "{}": {})", title(), Glfw::getError());

	// *** Set window properties ***

	// Real size might be different from requested size because of DPI scaling
	ivec2 realSize;
	glfwGetFramebufferSize(handle, &realSize.x, &realSize.y);
	if  (realSize == ivec2{0, 0})
		L.fail(R"(Failed to retrieve window "{}" framebuffer size: {})", _title, Glfw::getError());
	m_size = realSize;

	float realScale;
	glfwGetWindowContentScale(handle, &realScale, nullptr);
	m_scale = realScale;

	// *** Set up window callbacks ***

	glfwSetWindowUserPointer(handle, this);
#ifndef MINOTE_DEBUG
	glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif //MINOTE_DEBUG
	glfwSetKeyCallback(handle, keyCallback);
	glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(handle, windowScaleCallback);

	L.info(R"(Window "{}" created at {} *{}{})",
		title(), size(), scale(), fullscreen ? " fullscreen" : "");
}

Window::~Window() {
	DASSERT(!isContextActive);

	glfwDestroyWindow(handle);

	L.info(R"(Window "{}" closed)", title());
}

auto Window::isClosing() const -> bool {
	lock_guard const lock{handleMutex};
	return glfwWindowShouldClose(handle);
}

void Window::requestClose() {
	lock_guard const lock{handleMutex};
	glfwSetWindowShouldClose(handle, true);

	L.info(R"(Window "{}" close requested)", title());
}

void Window::flip() {
	glfwSwapBuffers(handle);
}

void Window::activateContext() {
	DASSERT(!isContextActive);
	DASSERT(!activeContext);

	glfwMakeContextCurrent(handle);
	isContextActive = true;
	activeContext = this;

	L.debug(R"(Window "{}" OpenGL context activated)", title());
}

void Window::deactivateContext() {
	DASSERT(isContextActive);
	DASSERT(activeContext == this);

	glfwMakeContextCurrent(nullptr);
	isContextActive = false;
	activeContext = nullptr;

	L.debug(R"(Window "{}" OpenGL context deactivated)", title());
}

void Window::popInput() {
	lock_guard const lock{inputsMutex};
	if (!inputs.empty())
		inputs.pop_front();
}

auto Window::getInput() const -> optional<KeyInput> {
	lock_guard const lock{inputsMutex};
	if (inputs.empty()) return nullopt;

	return inputs.front();
}

void Window::clearInput() {
	lock_guard const lock{inputsMutex};

	inputs.clear();
}

}
