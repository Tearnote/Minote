/**
 * System for creating windows with OpenGL contexts
 * @file
 * Open windows collect inputs in a thread-safe queue, and these inputs need
 * to be regularly collected. Additionally, the system needs to be regularly
 * polled with windowPoll() to keep windows responsive.
 */

#ifndef MINOTE_WINDOW_H
#define MINOTE_WINDOW_H

#include <stdbool.h>
#include "point.h"
#include "log.h"

/**
 * Opaque struct representing an open window. Obtain an instance with
 * windowCreate().
 */
typedef struct Window Window;

/// Struct containing information about a keypress event
typedef struct KeyInput {
	int key; /// GLFW keycode
	int action; /// GLFW_PRESS or GLFW_RELEASE
	//nsec timestamp; /// Time when the event was detected
} KeyInput;

/**
 * 	Collect pending events from the OS and keep every open ::Window responsive.
 * 	Call this as often as your target resolution of user input; at least 240Hz
 * 	is recommended.
 */
void windowPoll(void);

/**
 * Create a new ::Window instance, which represents a window on the screen.
 * The OpenGL context is inactive by default. Requires systemInit().
 * @param title The string to display on the title bar, also used in error
 * messages
 * @param size Size of the window in *logical* pixels. Affected by display DPI
 * @param fullscreen Fullscreen if true, windowed if false. A fullscreen window
 * is created at display resolution, ignoring the @a size parameter
 * @return A newly created ::Window. Needs to be destroyed with windowDestroy()
 */
Window* windowCreate(const char* title, Size2i size, bool fullscreen);

/**
 * Destroy a ::Window instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param w The ::Window object
 */
void windowDestroy(Window* w);

/**
 * Check whether a ::Window is open. If this returns false, the ::Window object
 * should be destroyed as soon as possible.
 * @param w The ::Window object
 * @return true if open, false if pending closure
 * @remark This function is thread-safe.
 */
bool windowIsOpen(Window* w);

/**
 * Set a ::Window's open flag to false. The window does not close immediately,
 * but is signaled to be destroyed as soon as possible by changing the return
 * value of windowIsOpen().
 * @param w The ::Window object
 * @remark This function is thread-safe.
 */
void windowClose(Window* w);

/**
 * Returns the title of a ::Window.
 * @param w The ::Window object
 * @return String displayed on the ::Window's title bar
 * @remark This function is thread-safe.
 */
const char* windowGetTitle(Window* w);

/**
 * Activate the ::Window's OpenGL context on the current thread. This is
 * required before OpenGL commands can be used. No other ::Window's context
 * can be active on the same thread, and windowContextDeactivate() must be
 * called before the ::Window is destroyed.
 * @param w The ::Window object
 */
void windowContextActivate(Window* w);

/**
 * Dectivate the ::Window's OpenGL context on the current thread. Must be
 * called on the same thread that activated it.
 * @param w The ::Window object
 */
void windowContextDeactivate(Window* w);

/**
 * Flip the ::Window's front and back buffers. Usually called by a ::Renderer
 * wishing to present its image to the screen.
 * @param w The ::Window object
 */
void windowFlip(Window* w);

/**
 * Remove and return a ::KeyInput from the ::Window's input queue. If the queue
 * is empty, nothing happens. Run this often to keep the queue from filling up
 * and discarding input events.
 * @param w The ::Window object
 * @param[out] input Object to rewrite with the removed input
 * @return true if successful, false if input queue is empty
 * @remark This function is thread-safe.
 */
bool windowInputDequeue(Window* w, KeyInput* input);

/**
 * Return a ::KeyInput from the ::Window's input queue without removing it. If
 * the queue is empty, nothing happens.
 * @param w The ::Window object
 * @param[out] input Object to rewrite with the peeked input
 * @return true if successful, false if input queue is empty
 * @remark This function is thread-safe.
 */
bool windowInputPeek(Window* w, KeyInput* input);

/**
 * Clear the ::Window's input queue.
 * @param w The ::Window object
 * @remark This function is thread-safe.
 */
void windowInputClear(Window* w);

#endif //MINOTE_WINDOW_H
