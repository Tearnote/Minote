/**
 * System for opening a window with an OpenGL context
 * @file
 * An open window collects inputs in a thread-safe queue, and these inputs need
 * to be regularly collected. Additionally, the system needs to be regularly
 * polled with windowPoll() to keep the window responsive.
 */

#ifndef MINOTE_WINDOW_H
#define MINOTE_WINDOW_H

#include <stdbool.h>
#include "visualtypes.h"

/// Struct containing information about a keypress event
typedef struct KeyInput {
	int key; ///< GLFW keycode
	int action; ///< GLFW_PRESS or GLFW_RELEASE
	//nsec timestamp; ///< Time when the event was detected
} KeyInput;

/**
 * Initialize the window system, showing the window with specified parameters
 * on the screen. The OpenGL context is inactive by default. Requires
 * systemInit().
 * @param title The string to display on the title bar
 * @param size Size of the window in *logical* pixels. Affected by display DPI
 * @param fullscreen Fullscreen if true, windowed if false. A fullscreen window
 * is created at display resolution, ignoring the @a size parameter
 */
void windowInit(const char* title, Size2i size, bool fullscreen);

/**
 * Close the open window and clean up the window system. No window function
 * can be used until windowInit() is called again.
 */
void windowCleanup(void);

/**
 * 	Collect pending events from the OS and keep the open window responsive.
 * 	Call this as often as your target resolution of user input; at least 240Hz
 * 	is recommended.
 */
void windowPoll(void);

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
Size2i windowGetSize(void);

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

#endif //MINOTE_WINDOW_H
