// Minote - window.h

#ifndef MINOTE_WINDOW_H
#define MINOTE_WINDOW_H

#include <string_view>
#include <functional>
#include <optional>
#include <mutex>
#include <queue>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "system.h"
#include "point.h"
#include "timer.h"

// Create an on-screen window with:
// - An (inactive) OpenGL 3.3 Core Profile context
// - A linear RGB framebuffer with 4xMSAA
// - DPI awareness
// - Pluggable keyboard event handling
class Window {
public:
	struct Input {
		int key;
		int action;
		nsec timestamp;
	};

	using KeyHandlerCallback = std::function<void(Input)>;

	// If fullscreen, size is ignored and window is created at current desktop resolution
	explicit Window(System& sys, std::string_view name = "Untitled",
			bool fullscreen = false, Size<int> size = defaultSize);
	~Window();

	// Returns false if a signal has been sent for the window to close.
	// When this happens, destroy this object ASAP.
	auto isOpen() -> bool;

	// Get the latest input from the window's queue, thread-safe
	auto popInput() -> std::optional<Input>;

private:
	static inline const Size<int> defaultSize{1280, 720};

	System& system;
	bool fullscreen;
	Size<int> size;
	double scale{0.0};
	GLFWwindow* window;

	std::mutex inputsMutex;
	std::queue<Input> inputs;

	// In these callbacks, the related Window object is retrieved through a user pointer inside the GLFWwindow
	static auto framebufferResizeCallback(GLFWwindow*, int w, int h) -> void;
	static auto windowScaleCallback(GLFWwindow*, float xScale, float yScale) -> void;
	static auto keyCallback(GLFWwindow*, int key, int scancode, int action, int mods) -> void;

	auto pushInput(Input) -> void;
};

#endif //MINOTE_WINDOW_H
