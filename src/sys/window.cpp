#include "window.hpp"

#include <optional>
#include <cassert>
#include <mutex>
#include "GLFW/glfw3.h"
#include "quill/Fmt.h"
#include "backends/imgui_impl_glfw.h"
#include "base/error.hpp"
#include "base/math.hpp"
#include "base/log.hpp"

namespace minote::sys {

using namespace base;

// Retrieve the Window from raw GLFW handle by user pointer.
static auto getWindow(GLFWwindow* _handle) -> Window& {
	
	assert(_handle);
	return *(Window*)(glfwGetWindowUserPointer(_handle));
	
}

void Window::keyCallback(GLFWwindow* _handle, int _rawKeycode, int _rawScancode, int _rawState,
	int) {
	
	assert(_handle);
	if (_rawState == GLFW_REPEAT) return; // Key repeat is not used
	auto& window = getWindow(_handle);
	using State = KeyInput::State;
	
	auto keycode = Keycode(_rawKeycode);
	auto scancode = Scancode(_rawScancode);
	auto name = window.m_glfw.getKeyName(keycode, scancode);
	auto state = _rawState == GLFW_PRESS? State::Pressed : State::Released;
	auto input = KeyInput{
		.keycode = keycode,
		.scancode = scancode,
		.name = name,
		.state = state,
		.timestamp = Glfw::getTime()};
	
	auto lock = std::scoped_lock(window.m_inputsMutex);
	window.m_inputs.push(input);
	
}

void Window::framebufferResizeCallback(GLFWwindow* _handle, int _width, int _height) {
	
	assert(_handle);
	assert(_width >= 0);
	assert(_height >= 0);
	auto& window = getWindow(_handle);
	
	auto newSize = ivec2{_width, _height};
	window.m_size = uvec2(newSize);
	
	L_INFO(R"(Window "{}" resized to {}x{})", window.title(), newSize.x(), newSize.y());
	
}

void Window::windowScaleCallback(GLFWwindow* _handle, float _xScale, float) {
	
	assert(_handle);
	assert(_xScale);
	// yScale seems to sometimes be 0.0, so it is not reliable
	auto& window = getWindow(_handle);
	
	window.m_scale = _xScale;
	
	L_INFO(R"(Window "{}" DPI scaling changed to {})", window.title(), _xScale);
	
}

void Window::cursorPosCallback(GLFWwindow* _handle, double _xPos, double _yPos) {
	
	assert(_handle);
	auto& window = getWindow(_handle);
	
	window.m_mousePos.store({f32(_xPos), f32(_yPos)});
	
}

void Window::mouseButtonCallback(GLFWwindow* _handle, int _button, int _action, int) {
	
	assert(_handle);
	auto& window = getWindow(_handle);
	
	if (_button == GLFW_MOUSE_BUTTON_LEFT)
		window.m_mouseDown.store(_action == GLFW_PRESS? true : false);
	
}

Window::Window(Glfw const& _glfw, std::string_view _title, bool _fullscreen, uvec2 _size):
	m_glfw(_glfw), m_title(_title) {
	
	assert(_size.x() > 0 && _size.y() > 0);
	
	// Set up context params
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // Declare DPI awareness
	
	// Create the window handle
	
	auto* monitor = glfwGetPrimaryMonitor();
	if (!monitor) throw runtime_error_fmt(
		"Failed to query primary monitor: {}", Glfw::getError());
	auto const* mode = glfwGetVideoMode(monitor);
	if (!mode) throw runtime_error_fmt(
		"Failed to query video mode: {}", Glfw::getError());
	if (_fullscreen)
		_size = uvec2(ivec2{mode->width, mode->height});
	else
		monitor = nullptr;
	
	m_handle = glfwCreateWindow(_size.x(), _size.y(), m_title.c_str(), monitor, nullptr);
	if (!m_handle) throw runtime_error_fmt(
		R"(Failed to init window "{}": {})", title(), Glfw::getError());
	
	// Set window properties
	
	// Real size might be different from requested size because of DPI scaling
	auto realSize = ivec2();
	glfwGetFramebufferSize(m_handle, &realSize.x(), &realSize.y());
	if (realSize == ivec2(0)) throw runtime_error_fmt(
		R"(Failed to retrieve window "{}" framebuffer size: {})",
		_title, Glfw::getError());
	m_size = uvec2(realSize);
	
	auto realScale = 0.0f;
	glfwGetWindowContentScale(m_handle, &realScale, nullptr);
	m_scale = realScale;
	
	// Set up window callbacks
	
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
		title(), size().x(), size().y(), scale(), _fullscreen ? " fullscreen" : "");
	
}

Window::~Window() {
	
	glfwDestroyWindow(m_handle);
	
	L_INFO(R"(Window "{}" closed)", title());
	
}

auto Window::isClosing() const -> bool {
	
	auto lock = std::scoped_lock(m_handleMutex);
	return glfwWindowShouldClose(m_handle);
	
}

void Window::requestClose() {
	
	auto lock = std::scoped_lock(m_handleMutex);
	if (glfwWindowShouldClose(m_handle)) return;
	
	glfwSetWindowShouldClose(m_handle, true);
	
	L_INFO(R"(Window "{}" close requested)", title());
	
}

}
