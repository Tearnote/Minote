/**
 * Facility for logging runtime events
 * @file
 * Supports log levels and multiple output targets per logger.
 */

#ifndef MINOTE_LOG_H
#define MINOTE_LOG_H

/**
 * Opaque logger type. You can obtain an instance with logCreate().
 */
typedef struct Log Log;

/// Log level enum, in ascending order of severity
/// @enum LogLevel
typedef enum LogLevel {
	LogNone, ///< zero value
	LogTrace, ///< logTrace()
	LogDebug, ///< logDebug()
	LogInfo, ///< logInfo()
	LogWarn, ///< logWarn()
	LogError, ///< logError()
	LogCrit, ///< logCrit()
	LogSize ///< terminator
} LogLevel;

/**
 * Initialize the log system. Needs to be called before any other log
 * functions.
 */
void logInit(void);

/**
 * Clean up the log system. All created logs need to be destroyed before
 * calling this function. No log function can be used until logInit() is
 * called again.
 */
void logCleanup(void);

/**
 * Create a ::Log instance with log level Info and all targets disabled.
 * @return The newly created ::Log. Needs to be destroyed with logDestroy()
 */
Log* logCreate(void);

/**
 * Destroy a ::Log instance. All enabled targets are flushed. The destroyed
 * object cannot be used anymore and the pointer becomes invalid.
 * @param l The ::Log object to destroy
 */
void logDestroy(Log* l);

/**
 * Enable the console log target. Messages at level Info and below will be
 * printed to stdout, messages at level Warn and above will be printed to
 * stderr.
 * @param l The ::Log object
 */
void logEnableConsole(Log* l);

/**
 * Enable the file log target. The destination file is cleared. If the file
 * could not be opened, console is enabled instead and a Warn message is
 * printed. Does nothing if file logging is already enabled.
 * @param l The ::Log object
 * @param filepath Path to the log file to open for writing
 */
void logEnableFile(Log* l, const char* filepath);

/**
 * Disable the console log target.
 * @param l The ::Log object
 */
void logDisableConsole(Log* l);

/**
 * Disable the file log target. The associated file is flushed and closed.
 * @param l The ::Log object
 */
void logDisableFile(Log* l);

/**
 * Change the log level of a ::Log object. Messages below this severity will
 * be ignored.
 * @param l The ::Log object
 * @param level The least severe message level to display
 */
void logSetLevel(Log* l, LogLevel level);

/**
 * %Log a message at Trace level to all enabled targets. This level is for
 * active debugging purposes only and no logTrace() should be present in
 * shipped code.
 * @param l The ::Log object
 * @param fmt Format string in printf syntax
 * @param ... Any number of arguments to print
 */
void logTrace(Log* l, const char* fmt, ...);

/**
 * %Log a message at Debug level to all enabled targets. This level is for
 * messages that aid a developer trying to diagnose a problem.
 * @param l The ::Log object
 * @param fmt Format string in printf syntax
 * @param ... Any number of arguments to print
 */
void logDebug(Log* l, const char* fmt, ...);

/**
 * %Log a message at Info level to all enabled targets. This level is for
 * lifecycle information that makes sense to an end user.
 * @param l The ::Log object
 * @param fmt Format string in printf syntax
 * @param ... Any number of arguments to print
 */
void logInfo(Log* l, const char* fmt, ...);

/**
 * %Log a message at Warn level to all enabled targets. This level is for
 * degradation in functionality (for example, failed to open an audio device).
 * @param l The ::Log object
 * @param fmt Format string in printf syntax
 * @param ... Any number of arguments to print
 */
void logWarn(Log* l, const char* fmt, ...);

/**
 * %Log a message at Error level to all enabled targets. This level is for
 * complete loss of functionality (for example, could not enter replay mode).
 * @param l The ::Log object
 * @param fmt Format string in printf syntax
 * @param ... Any number of arguments to print
 */
void logError(Log* l, const char* fmt, ...);

/**
 * %Log a message at Crit level to all enabled targets. This level is for
 * situations that cannot be recovered from and execution needs to be aborted.
 * Call logDestroy() before aborting to make sure the message is written.
 * @param l The ::Log object
 * @param fmt Format string in printf syntax
 * @param ... Any number of arguments to print
 */
void logCrit(Log* l, const char* fmt, ...);

#endif //MINOTE_LOG_H
