#include "window.hpp"

#include <string_view>
#include <stdexcept>
#include <optional>
#include <cassert>
#include <mutex>
#include "GLFW/glfw3.h"
#include "quill/Fmt.h"
#include "backends/imgui_impl_glfw.h"
#include "base/math.hpp"
#include "base/log.hpp"

namespace minote::sys {

using namespace base;

// Retrieve the Window from raw GLFW handle by user pointer.
static auto getWindow(GLFWwindow* handle) -> Window& {
	assert(handle);
	return *(Window*)(glfwGetWindowUserPointer(handle));
}

void Window::keyCallback(GLFWwindow* handle, int rawKeycode, int rawScancode, int rawState,
	int) {
	assert(handle);
	if (rawState == GLFW_REPEAT) return; // Key repeat is not used
	auto& window = getWindow(handle);
	using State = KeyInput::State;

	auto keycode = Keycode(rawKeycode);
	auto scancode = Scancode(rawScancode);
	auto name = window.glfw.getKeyName(keycode, scancode);
	auto state = rawState == GLFW_PRESS? State::Pressed : State::Released;
	auto input = KeyInput{
		.keycode = keycode,
		.scancode = scancode,
		.name = name,
		.state = state,
		.timestamp = Glfw::getTime()
	};

	auto lock = std::scoped_lock(window.inputsMutex);
	try {
		window.inputs.push(input);
	} catch (...) {
		L_WARN(R"(Window "{}" input queue full, key "{}" {} event dropped)",
			window.title(), name, state == State::Pressed? "press" : "release");
	}
}

void Window::framebufferResizeCallback(GLFWwindow* handle, int width, int height) {
	assert(handle);
	assert(width >= 0);
	assert(height >= 0);
	auto& window = getWindow(handle);

	auto newSize = ivec2{width, height};
	window.m_size = uvec2(newSize);
	L_INFO(R"(Window "{}" resized to {}x{})", window.title(), newSize.x(), newSize.y());
}

// Function to run when the window is rescaled. This might happen when dragging
// it to a display with different DPI scaling, or at startup. The new scale
// is saved for later retrieval.
void Window::windowScaleCallback(GLFWwindow* handle, float xScale, float) {
	assert(handle);
	assert(xScale);
	// yScale seems to sometimes be 0.0, so it is not reliable
	auto& window = getWindow(handle);

	window.m_scale = xScale;
	L_INFO(R"(Window "{}" DPI scaling changed to {})", window.title(), xScale);
}

void Window::cursorPosCallback(GLFWwindow* handle, double xPos, double yPos) {
	assert(handle);
	auto& window = getWindow(handle);

	window.m_mousePos.store({f32(xPos), f32(yPos)});
}

void Window::mouseButtonCallback(GLFWwindow* handle, int button, int action, int) {
	assert(handle);
	auto& window = getWindow(handle);

	if (button == GLFW_MOUSE_BUTTON_LEFT)
		window.m_mouseDown.store(action == GLFW_PRESS? true : false);
}

Window::Window(Glfw const& _glfw, std::string_view _title, bool fullscreen, uvec2 _size):
	glfw(_glfw), m_title(_title) {
	assert(_size.x() > 0 && _size.y() > 0);

	// *** Set up context params ***

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // Declare DPI awareness

	// *** Create the window handle ***

	auto* monitor = glfwGetPrimaryMonitor();
	if (!monitor)
		throw std::runtime_error(
			fmt::format("Failed to query primary monitor: {}", Glfw::getError()));
	auto const* mode = glfwGetVideoMode(monitor);
	if (!mode)
		throw std::runtime_error(
			fmt::format("Failed to query video mode: {}", Glfw::getError()));
	if (fullscreen)
		_size = uvec2(ivec2{mode->width, mode->height});
	else
		monitor = nullptr;

	m_handle = glfwCreateWindow(_size.x(), _size.y(), m_title.c_str(), monitor, nullptr);
	if (!m_handle)
		throw std::runtime_error(
			fmt::format(R"(Failed to init window "{}": {})", title(), Glfw::getError()));

	// *** Set window properties ***

	// Real size might be different from requested size because of DPI scaling
	auto realSize = ivec2();
	glfwGetFramebufferSize(m_handle, &realSize.x(), &realSize.y());
	if (realSize == ivec2(0))
		throw std::runtime_error(
			fmt::format(R"(Failed to retrieve window "{}" framebuffer size: {})", _title,
				Glfw::getError()));
	m_size = uvec2(realSize);

	auto realScale = 0.0f;
	glfwGetWindowContentScale(m_handle, &realScale, nullptr);
	m_scale = realScale;

	// *** Set up window callbacks ***

	glfwSetWindowUserPointer(m_handle, this);
	glfwSetKeyCallback(m_handle, keyCallback);
	glfwSetFramebufferSizeCallback(m_handle, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(m_handle, windowScaleCallback);
	glfwSetCursorPosCallback(m_handle, cursorPosCallback);
	glfwSetMouseButtonCallback(m_handle, mouseButtonCallback);

	// Initialize imgui input
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(m_handle, true);

	L_INFO(R"(Window "{}" created at {}x{} *{:.2f}{})",
		title(), size().x(), size().y(), scale(), fullscreen ? " fullscreen" : "");
}

Window::~Window() {
	glfwDestroyWindow(m_handle);

	L_INFO(R"(Window "{}" closed)", title());
}

auto Window::isClosing() const -> bool {
	auto lock = std::scoped_lock(handleMutex);
	return glfwWindowShouldClose(m_handle);
}

void Window::requestClose() {
	auto lock = std::scoped_lock(handleMutex);
	if (glfwWindowShouldClose(m_handle)) return;

	glfwSetWindowShouldClose(m_handle, true);

	L_INFO(R"(Window "{}" close requested)", title());
}

}
