#include "window.hpp"

#include <string_view>
#include <stdexcept>
#include <optional>
#include <mutex>
#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/log.hpp"

namespace minote::sys {

using namespace base;

// Retrieve the Window from raw GLFW handle by user pointer.
static auto getWindow(GLFWwindow* handle) -> Window& {
	ASSERT(handle);
	return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(handle));
}

void Window::keyCallback(GLFWwindow* handle, int rawKeycode, int rawScancode, int rawState,
	int) {
	ASSERT(handle);
	if (rawState == GLFW_REPEAT) return; // Key repeat is not used
	auto& window = getWindow(handle);
	using State = KeyInput::State;

	auto const keycode = Keycode{rawKeycode};
	auto const scancode = Scancode{rawScancode};
	auto const name = window.glfw.getKeyName(keycode, scancode);
	auto const state = rawState == GLFW_PRESS? State::Pressed : State::Released;
	auto const input = KeyInput{
		.keycode = keycode,
		.scancode = scancode,
		.name = name,
		.state = state,
		.timestamp = Glfw::getTime()
	};

	auto const lock = std::scoped_lock{window.inputsMutex};
	try {
		window.inputs.push_back(input);
	} catch (...) {
		L.warn(R"(Window "{}" input queue full, key "{}" {} event dropped)",
			window.title(), name, state == State::Pressed? "press" : "release");
	}
}

void Window::framebufferResizeCallback(GLFWwindow* handle, int width, int height) {
	ASSERT(handle);
	ASSERT(width >= 0);
	ASSERT(height >= 0);
	auto& window = getWindow(handle);

	auto const newSize = glm::uvec2{width, height};
	window.m_size = newSize;
	L.info(R"(Window "{}" resized to {:s})", window.title(), newSize);
}

// Function to run when the window is rescaled. This might happen when dragging
// it to a display with different DPI scaling, or at startup. The new scale
// is saved for later retrieval.
void Window::windowScaleCallback(GLFWwindow* handle, float xScale, float) {
	ASSERT(handle);
	ASSERT(xScale);
	// yScale seems to sometimes be 0.0, so it is not reliable
	auto& window = getWindow(handle);

	window.m_scale = xScale;
	L.info(R"(Window "{}" DPI scaling changed to {})", window.title(), xScale);
}

Window::Window(Glfw const& _glfw, std::string_view _title, bool fullscreen, glm::uvec2 _size):
	glfw{_glfw}, m_title{_title} {
	ASSERT(_size.x > 0 && _size.y > 0);

	// *** Set up context params ***

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // Declare DPI awareness

	// *** Create the window handle ***

	auto* monitor = glfwGetPrimaryMonitor();
	if (!monitor)
		throw std::runtime_error{
			fmt::format("Failed to query primary monitor: {}", Glfw::getError())};
	auto const* const mode = glfwGetVideoMode(monitor);
	if (!mode)
		throw std::runtime_error{
			fmt::format("Failed to query video mode: {}", Glfw::getError())};
	if (fullscreen)
		_size = {mode->width, mode->height};
	else
		monitor = nullptr;

	m_handle = glfwCreateWindow(_size.x, _size.y, m_title.c_str(), monitor, nullptr);
	if (!m_handle)
		throw std::runtime_error{
			fmt::format(R"(Failed to init window "{}": {})", title(), Glfw::getError())};

	// *** Set window properties ***

	// Real size might be different from requested size because of DPI scaling
	glm::ivec2 realSize;
	glfwGetFramebufferSize(m_handle, &realSize.x, &realSize.y);
	if (realSize == glm::ivec2{0, 0})
		throw std::runtime_error{
			fmt::format(R"(Failed to retrieve window "{}" framebuffer size: {})", _title,
				Glfw::getError())};
	m_size = realSize;

	float realScale;
	glfwGetWindowContentScale(m_handle, &realScale, nullptr);
	m_scale = realScale;

	// *** Set up window callbacks ***

	glfwSetWindowUserPointer(m_handle, this);
#ifndef MINOTE_DEBUG
	glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif //MINOTE_DEBUG
	glfwSetKeyCallback(m_handle, keyCallback);
	glfwSetFramebufferSizeCallback(m_handle, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(m_handle, windowScaleCallback);

	L.info(R"(Window "{}" created at {:s} *{:.2f}{})",
		title(), size(), scale(), fullscreen ? " fullscreen" : "");
}

Window::~Window() {
	glfwDestroyWindow(m_handle);

	L.info(R"(Window "{}" closed)", title());
}

auto Window::isClosing() const -> bool {
	auto const lock = std::scoped_lock{handleMutex};
	return glfwWindowShouldClose(m_handle);
}

void Window::requestClose() {
	auto const lock = std::scoped_lock{handleMutex};
	if (glfwWindowShouldClose(m_handle)) return;

	glfwSetWindowShouldClose(m_handle, true);

	L.info(R"(Window "{}" close requested)", title());
}

}
