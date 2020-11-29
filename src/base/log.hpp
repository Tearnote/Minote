// Minote - base/log.hpp
// Facility for logging runtime events. Supports log levels and multiple output
// targets per logger. Blocking and non-threaded, so don't use in large volumes.

#pragma once

#include "base/util.hpp"
#include "base/io.hpp"

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

	// Enable logging to a file. The file will be created if missing.
	void enableFile(path const& file);

	// Disable logging to a file, cleanly closing any currently open logfile.
	void disableFile();

	// Log a Trace level message. Meant for "printf debugging", do not leave any
	// trace() calls in committed code.
	// Example: pos.x = 3, pos.y = 7
	template<typename S, typename... Args>
	void trace(const S& fmt, Args&&... args);

	// Log a Debug level message. Meant for diagnostic information that only
	// makes sense to a developer.
	// Example: Shader compilation successful
	template<typename S, typename... Args>
	void debug(const S& fmt, Args&&... args);

	// Log an Info level message. Meant for information that is understandable
	// by an inquisitive end user.
	// Example: Player settings saved to database
	template<typename S, typename... Args>
	void info(const S& fmt, Args&&... args);

	// Log a Warn level message. Meant for failures that cause a subsystem
	// to run in a limited capacity.
	// Example: Could not load texture
	template<typename S, typename... Args>
	void warn(const S& fmt, Args&&... args);

	// Log an Error level message. Meant for failures that a subsystem cannot
	// recover from.
	// Example: Could not find any audio device
	template<typename S, typename... Args>
	void error(const S& fmt, Args&&... args);

	// Log a Crit level message. Meant for failures that the entire application
	// cannot recover from.
	// Example: Failed to initialize OpenGL
	template<typename S, typename... Args>
	void crit(const S& fmt, Args&&... args);

	// Log a Crit level message and exit the application immediately.
	template<typename S, typename... Args>
	[[noreturn]]
	void fail(const S& fmt, Args&&... args);

	template<typename S, typename... Args>
	void log(Log::Level level, S const& fmt, Args&&... args);

	// Clean up by closing any open logfile.
	~Log();

};

// Global logger available for convenience
inline Log L;

}

#include "log.tpp"
