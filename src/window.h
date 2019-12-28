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
 * Initialize the window system. Needs to be called before any other window
 * functions.
 * @param log The ::Log to use for logging during the window system's runtime.
 * Should not be destroyed until after windowCleanup()
 */
void windowInit(Log* log);

/**
 * Clean up the window system. All ::Window instances must be destroyed first.
 * No window function can be used until windowInit() is called again.
 */
void windowCleanup(void);

/**
 * 	Collect pending events from the OS and keep every open ::Window responsive.
 * 	Call this as often as your target resolution of user input; at least 240Hz
 * 	is recommended.
 */
void windowPoll(void);

/**
 * Create a new ::Window instance, which represents a window on the screen.
 * The OpenGL context is inactive by default.
 * @param title The string to display on the title bar, also used in error
 * messages
 * @param size Size of the window in *logical* pixels. Affected by display DPI
 * @param fullscreen Fullscreen if true, windowed if false. A fullscreen window
 * is created at display resolution, ignoring the size parameter
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
 * Check whether the window is open. If this returns false, the ::Window object
 * should be destroyed as soon as possible.
 * @param w The ::Window object
 * @return true if open, false if pending closure
 */
bool windowIsOpen(Window* w);

#endif //MINOTE_WINDOW_H
