#ifndef LOG_H
#define LOG_H

#define PRIO_DEBUG 1 // Information useful to the developer
                         // For example: value of a variable
#define PRIO_INFO  2 // Information useful to the user
                         // For example: current video mode
#define PRIO_WARN  3 // Degradation of functionality
                         // For example: could not load an image
#define PRIO_ERROR 4 // Loss of functionality
                         // For example: failed to initialize audio
#define PRIO_CRIT  5 // Cannot reasonably continue
                        // For example: could not open window

void initLogging();
void cleanupLogging();

// Standard printf syntax
void logPrio(int prio, const char* fmt, ...);
#define logDebug(...) logPrio(PRIO_DEBUG, __VA_ARGS__)
#define logInfo(...) logPrio(PRIO_INFO, __VA_ARGS__)
#define logWarn(...) logPrio(PRIO_WARN, __VA_ARGS__)
#define logError(...) logPrio(PRIO_ERROR, __VA_ARGS__)
#define logCrit(...) logPrio(PRIO_CRIT, __VA_ARGS__)

// Convenience functions for inserting the GLFW error after the message
void logPrioGLFW(int prio, const char* msg);
#define logWarnGLFW(msg) logPrioGLFW(PRIO_WARN, (msg))
#define logErrorGLFW(msg) logPrioGLFW(PRIO_ERROR, (msg))
#define logCritGLFW(msg) logPrioGLFW(PRIO_CRIT, (msg))

#endif