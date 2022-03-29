#include "window.hpp"

#include <optional>
#include <cassert>
#include <mutex>
#include "SDL_vulkan.h"
#include "SDL_video.h"
#include "quill/Fmt.h"
#include "backends/imgui_impl_sdl.h"
#include "base/error.hpp"
#include "base/math.hpp"
#include "base/log.hpp"

namespace minote::sys {

using namespace base;
/*
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
*/
Window::Window(System const& _system, string_view _title, bool _fullscreen, uvec2 _size):
	m_system(_system), m_title(_title) {
	
	assert(_size.x() > 0 && _size.y() > 0);
	
	// Create window
	
	m_handle = SDL_CreateWindow(m_title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		_size.x(), _size.y(),
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_ALLOW_HIGHDPI |
		SDL_WINDOW_VULKAN |
		(_fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	if(!m_handle)
		throw runtime_error_fmt("Failed to create window {}: {}", m_title, SDL_GetError());
	
	// Set window properties
	
	// Real size might be different from requested size because of DPI scaling
	auto realSize = ivec2();
	SDL_Vulkan_GetDrawableSize(m_handle, &realSize.x(), &realSize.y());
	m_size = uvec2(realSize);
	
	auto dpi = 0.0f;
	SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(m_handle), nullptr, nullptr, &dpi);
	m_dpi = dpi;
	
	// Initialize imgui input
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForVulkan(m_handle);
	
	L_INFO("Window {} created at {}x{}, {} DPI{}",
		m_title, realSize.x(), realSize.y(), dpi, _fullscreen? ", fullscreen" : "");
	
}

Window::~Window() {
	
	ImGui_ImplSDL2_Shutdown();
	
	SDL_DestroyWindow(m_handle);
	
	L_INFO("Window {} closed", title());
	
}

}
