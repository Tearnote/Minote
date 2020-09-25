/**
 * Main system of interaction with the OS
 * @file
 * Initializes the GLFW library and possibly other major OS interfaces.
 */

#ifndef MINOTE_SYSTEM_H
#define MINOTE_SYSTEM_H

/**
 * Initialize the main system (GLFW). Needs to be called before most other
 * systems can be used.
 */
void systemInit(void);

/**
 * Clean up the main system. All instances of objects obtained from other
 * systems must be destroyed first. Other systems' functions cannot be used
 * until systemInit() is called again.
 */
void systemCleanup(void);

/**
 * Return the last system error message and clear error state. Does not require
 * systemInit().
 * @return String literal describing the error. Use immediately, before calling
 * any other GLFW functions
 * @remark This function is thread-safe - only errors from the calling thread
 * are reported.
 */
const char* systemError(void);

#endif //MINOTE_SYSTEM_H
