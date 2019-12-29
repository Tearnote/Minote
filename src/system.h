/**
 * Main system of interaction with the OS
 * @file
 */

#ifndef MINOTE_SYSTEM_H
#define MINOTE_SYSTEM_H

#include "log.h"

/// Log file to be used by the main and related systems
extern Log* syslog;

/**
 * Initialize the main system. Needs to be called before most other systems can
 * be used.
 * @param log The ::Log to use for logging during the main system's runtime.
 * Should not be destroyed until after systemCleanup()
 */
void systemInit(Log* log);

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
 */
const char* systemError(void);

#endif //MINOTE_SYSTEM_H
