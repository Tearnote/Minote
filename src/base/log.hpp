/**
 * Facility for logging runtime events
 * @file
 * Supports log levels and multiple output targets per logger.
 */

#pragma once

#include <cstdio>

namespace minote {

struct Log {

	/// Logging level, in order of increasing importance
	enum struct Level {
		None, ///< zero value
		Trace, ///< trace()
		Debug, ///< debug()
		Info, ///< info()
		Warn, ///< warn()
		Error, ///< error()
		Crit, ///< crit()
		Size ///< terminator
	};

	/// Messages with level lower than this will be ignored
	Level level = Level::None;

	/// If true, messages are printed to stdout/stderr
	bool console = false;

	/// File handle to write messages into, or nullptr for disabled file logging
	FILE* file = nullptr;

	~Log();

	/**
	 * Enable logging to a file. Remember to call disableFile() when finished,
	 * so that the file can be closed properly.
	 * @param filepath Path to the logfile. File does not have to already exist
	 */
	void enableFile(const char* filepath);

	/**
	 * Disable logging to a file, cleanly closing the currently open logfile.
	 */
	void disableFile();

	/**
	 * Log a Trace level message. Meant for "printf debugging", do not leave any
	 * trace() calls in committed code.
	 * Example: pos.x = 3, pos.y = 7
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]]
	void trace(const char* fmt, ...);

	/**
	 * Log a Debug level message. Meant for diagnostic information
	 * that only makes sense to a developer.
	 * Example: Shader compilation successful
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]]
	void debug(const char* fmt, ...);

	/**
	 * Log an Info level message. Meant for information that is understandable
	 * by an inquisitive end user.
	 * Example: Player settings saved to database
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]]
	void info(const char* fmt, ...);

	/**
	 * Log a Warn level message. Meant for failures that cause a subsystem
	 * to run in a limited capacity.
	 * Example: Could not load texture
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]]
	void warn(const char* fmt, ...);

	/**
	 * Log an Error level message. Meant for failures that a subsystem cannot
	 * recover from.
	 * Example: Could not find any audio device
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]]
	void error(const char* fmt, ...);

	/**
	 * Log a Crit level message. Meant for failures that the entire application
	 * cannot recover from.
	 * Example: Failed to initialize OpenGL
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]]
	void crit(const char* fmt, ...);

	/**
	 * Log a Crit level message and exit the application immediately.
	 * @param fmt Format string in printf syntax
	 * @param ... String interpolation arguments
	 */
	[[gnu::format(printf, 2, 3)]] [[noreturn]]
	void fail(const char* fmt, ...);

};

/// Global logger available for convenience
inline Log L;

}
