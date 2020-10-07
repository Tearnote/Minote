/**
 * System for opening a window with an OpenGL context
 * @file
 * An open window collects collectedInputs in a thread-safe queue, and these collectedInputs need
 * to be regularly collected. Additionally, the system needs to be regularly
 * polled with windowPoll() to keep the window responsive.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <GLFW/glfw3.h>
#include "base/types.hpp"
#include "base/time.hpp"

/// Struct containing information about a keypress event
typedef struct KeyInput {
	int key; ///< GLFW keycode
	int action; ///< GLFW_PRESS or GLFW_RELEASE
	minote::nsec timestamp; ///< Time when the event was detected
} KeyInput;

namespace minote {

struct Window {

	inline static bool initialized = false;

	/**
	 * Initialize the windowing system and other OS-specific bits. This must
	 * be called before any windows are opened.
	 * @remark The thread that calls this becomes the "main" thread.
	 */
	static void init();

	/**
	 * Clean up the windowing system. All windows must already be closed.
	 * @remark Must be called on the main thread.
	 */
	static void cleanup();

	/**
	 * Collect pending events for all open windows and keep them responsive.
	 * Call this as often as your target resolution of user input; at least
	 * 240Hz is recommended.
	 * @remark Must be called on the main thread.
	 */
	static void poll();

	/**
	 * Return the time passed since Window::init(). If init() was not called,
	 * 0 is returned.
	 * @return Number of nanoseconds since windowing initialization
	 * @remark This function is thread-safe.
	 */
	static auto getTime() -> nsec;

	////////////////////////////////////////////////////////////////////////////

	/// Raw window handle. Please take note of thread safety notes in GLFW
	/// documentation when using this field directly.
	GLFWwindow* handle = nullptr;
	/// Mutex protecting handle access with unsynchronized GLFW functions
	std::mutex handleMutex;

	const char* title = nullptr; ///< Text displayed on the window's title bar
	std::atomic<size2i> size; ///< Size of the window in physical pixels
	std::atomic<float> scale = 0.0f; ///< DPI scaling, where 1.0 is "standard" DPI
	bool isContextActive = false; ///< Whether the OpenGL context is active on any thread

	/**
	 * Open a window with specified parameters on the screen. OpenGL context
	 * is not activated.
	 * @param title Text to display on the title bar
	 * @param fullscreen false for windowed mode, true for fullscreen
	 * @param size Size of the window in logical units. If fullscreen is true,
	 * this parameter is ignored and the window is created at desktop resolution
	 * @remark This function must be called on the main thread.
	 */
	void open(const char* title = "", bool fullscreen = false, size2i size = {1280, 720});

	/**
	 * Close an open window. The OpenGL context must be already deactivated.
	 * @remark This function must be called on the main thread.
	 */
	void close();

	/**
	 * Ask if window close has been requested by application or user
	 * (for example by pressing the X on the title bar). If true, the window
	 * should be closed as soon as possible.
	 * @return true if close requested, false if not
	 * @remark This function is thread-safe.
	 */
	[[nodiscard]]
	auto isClosing() -> bool;

	/**
	 * Request the window to be closed by the main thread.
	 * @remark This function is thread-safe.
	 */
	void requestClose();

	/**
	 * Flip the window's front and back buffers. Call this after a frame
	 * is drawn to present it on the screen.
	 * @remark This function is thread-safe.
	 */
	void flip();

	/**
	 * Activate the window's OpenGL context on current thread. This is required
	 * before OpenGL commands can be used. A window's context can be active
	 * on one thread at a time, and a thread can have only one window's context
	 * active.
	 * @remark This function is thread-safe.
	 */
	void activateContext();

	/**
	 * Deactivate the window's OpenGL context on current thread. The context
	 * must be inactive before the window can be closed.
	 * @remark This function is thread-safe.
	 */
	void deactivateContext();

};

}

/**
 * Remove and return a ::KeyInput from the window's input queue. If the queue
 * is empty, nothing happens. Run this often to keep the queue from filling up
 * and discarding input events.
 * @param[out] input Object to rewrite with the removed input
 * @return true if successful, false if input queue is empty
 * @remark This function is thread-safe.
 */
bool windowInputDequeue(KeyInput* input);

/**
 * Return a ::KeyInput from the window's input queue without removing it. If
 * the queue is empty, nothing happens.
 * @param[out] input Object to rewrite with the peeked input
 * @return true if successful, false if input queue is empty
 * @remark This function is thread-safe.
 */
bool windowInputPeek(KeyInput* input);

/**
 * Clear the window's input queue. This can remove a GLFW_RELEASE event, so
 * consider every key unpressed.
 * @remark This function is thread-safe.
 */
void windowInputClear(void);
