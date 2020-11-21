// Minote - base/log.hpp
// Facility for logging runtime events. Supports log levels and multiple output
// targets per logger. Blocking and non-threaded, so don't use in large volumes.

#pragma once

#include <cstdio>

namespace minote {

struct Log {

	// Logging level. Messages with level lower than this will be ignored
	enum struct Level {
		None,
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Crit,
		Size
	} level = Level::None;

	// If true, messages are printed to stdout (<=Info) or stderr (>=Warn)
	bool console = false;

	// File handle to write messages into, or nullptr for disabled file logging
	FILE* file = nullptr;

	// Enable logging to a file. The file will be created if missing. Remember
	// to call disableFile() when finished, so that the file can be closed
	// properly.
	void enableFile(char const* filepath);

	// Disable logging to a file, cleanly closing the currently open logfile.
	void disableFile();

	// Log a Trace level message. Meant for "printf debugging", do not leave any
	// trace() calls in committed code.
	// Example: pos.x = 3, pos.y = 7
	[[gnu::format(printf, 2, 3)]]
	void trace(char const* fmt, ...);

	// Log a Debug level message. Meant for diagnostic information that only
	// makes sense to a developer.
	// Example: Shader compilation successful
	[[gnu::format(printf, 2, 3)]]
	void debug(char const* fmt, ...);

	// Log an Info level message. Meant for information that is understandable
	// by an inquisitive end user.
	// Example: Player settings saved to database
	[[gnu::format(printf, 2, 3)]]
	void info(char const* fmt, ...);

	// Log a Warn level message. Meant for failures that cause a subsystem
	// to run in a limited capacity.
	// Example: Could not load texture
	[[gnu::format(printf, 2, 3)]]
	void warn(char const* fmt, ...);

	// Log an Error level message. Meant for failures that a subsystem cannot
	// recover from.
	// Example: Could not find any audio device
	[[gnu::format(printf, 2, 3)]]
	void error(char const* fmt, ...);

	// Log a Crit level message. Meant for failures that the entire application
	// cannot recover from.
	// Example: Failed to initialize OpenGL
	[[gnu::format(printf, 2, 3)]]
	void crit(char const* fmt, ...);

	// Log a Crit level message and exit the application immediately.
	[[gnu::format(printf, 2, 3)]] [[noreturn]]
	void fail(char const* fmt, ...);

	~Log();

};

// Global logger available for convenience
inline Log L;

}
