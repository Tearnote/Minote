/**
 * System for opening a window with an OpenGL context
 * @file
 * An open window collects collectedInputs in a thread-safe queue, and these collectedInputs need
 * to be regularly collected. Additionally, the system needs to be regularly
 * polled with windowPoll() to keep the window responsive.
 */

#pragma once

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

};

}

/**
 * Check whether the window is open. If this returns false, windowCleanup()
 * should be called as soon as possible.
 * @return true if open, false if pending closure
 * @remark This function is thread-safe.
 */
bool windowIsOpen(void);

/**
 * Set the window's open flag to false. The window does not close immediately,
 * but is signaled to be destroyed as soon as possible by changing the return
 * value of windowIsOpen().
 * @remark This function is thread-safe.
 */
void windowClose(void);

/**
 * Returns the title of the window.
 * @return String displayed on the window's title bar
 * @remark This function is thread-safe.
 */
const char* windowGetTitle(void);

/**
 * Return the size of the window in pixels.
 * @return Size of the window in pixels
 * @remark This function is thread-safe.
 */
minote::size2i windowGetSize(void);

/**
 * Return the scale of the window, with 1.0 being "normal".
 * @return Scale of the window
 * @remark This function is thread-safe.
 */
float windowGetScale(void);

/**
 * Activate the window's OpenGL context on the current thread. This is
 * required before OpenGL commands can be used. windowContextDeactivate()
 * must be called before windowCleanup().
 */
void windowContextActivate(void);

/**
 * Dectivate the window's OpenGL context on the current thread. Must be
 * called on the same thread that activated it.
 */
void windowContextDeactivate(void);

/**
 * Flip the window's front and back buffers. Call after a frame is drawn to
 * present it on the screen.
 */
void windowFlip(void);

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

typedef struct GLFWwindow GLFWwindow;
/**
 * Return a handle to the GLFW window. Use this very carefully, because most
 * GLFW functions are not thread-safe.
 * @return GLFW window pointer
 */
GLFWwindow* getRawWindow(void);
