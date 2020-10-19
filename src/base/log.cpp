/**
 * Implementation of log.hpp
 * @file
 */

#include "log.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "util.hpp"

namespace minote {

/// Mapping from Log::Level to string name
static constexpr char LogLevelStrings[][+Log::Level::Size] = {
	"", "TRACE", "DEBUG", " INFO", " WARN", "ERROR", " CRIT"
};

/// Messages longer than this will be truncated
static constexpr std::size_t MaxMessageLen = 2048;

/**
 * Write a preformatted log message to a specified output.
 * @param file The output file
 * @param msg Preformatted message, with a newline at the end
 */
static void logTo(FILE* const file, char const msg[])
{
	ASSERT(msg);

	if (std::fputs(msg, file) == EOF) {
		std::perror("Failed to write into logfile");
		errno = 0;
	}
}

/**
 * The actual log message handling function. Performs level filtering,
 * formatting and output selection.
 * @param log The ::Log object
 * @param level Message level
 * @param fmt Format string in printf syntax
 * @param ap List of arguments to print
 */
static void logPrio(Log& log, Log::Level const level, char const* const fmt, va_list& ap)
{
	ASSERT(fmt);
	using Level = Log::Level;

	if (level < log.level)
		return;
	if (!log.console && !log.file)
		return;

	char msg[MaxMessageLen] = {};

	// Insert timestamp and level
	auto const now = std::time(nullptr);
	// Copy-initialize to minimize chance of thread safety issues
	auto const localnow = *std::localtime(&now);
	std::snprintf(msg, MaxMessageLen, "%02d:%02d:%02d [%s] ",
		localnow.tm_hour, localnow.tm_min, localnow.tm_sec,
		LogLevelStrings[+level]);

	// Insert formatted message
	auto const timestampLen = std::strlen(msg);
	std::vsnprintf(msg + timestampLen, MaxMessageLen - timestampLen, fmt, ap);

	// Append a newline
	msg[std::min(std::strlen(msg), MaxMessageLen - 2)] = '\n';

	// Print constructed message to all enabled targets
	if (log.console) {
		if (level >= Level::Warn) {
			// Ensure previously written messages are not interleaved
			std::fflush(stdout);
			logTo(stderr, msg);
			// Ensure message is written, in case the application crashes
			std::fflush(stderr);
		} else {
			logTo(stdout, msg);
		}
	}

	if (log.file) {
		logTo(log.file, msg);
		// Ensure message is written, in case the application crashes
		if (level >= Level::Warn) {
			if (std::fflush(log.file) == EOF) {
				std::perror("Failed to write into logfile");
				errno = 0;
			}
		}
	}
}

void Log::enableFile(char const* const filepath)
{
	ASSERT(filepath);
	if (file) {
		warn("Not opening logfile %s: already logging to a file", filepath);
		return;
	}

	file = std::fopen(filepath, "w");
	if (file)
		return;

	// Handle error by forcibly printing it to console
	auto const consolePrev = console;
	console = true;
	error("Failed to open logfile %s for writing: %s",
		filepath, std::strerror(errno));
	errno = 0;
	console = consolePrev;
}

void Log::disableFile()
{
	if (!file)
		return;

	auto const result = fclose(file);
	file = nullptr;
	if (!result)
		return;

	// Handle error by forcibly printing it to console
	auto const consolePrev = console;
	console = true;
	error("Failed to flush logfile: %s", std::strerror(errno));
	errno = 0;
	console = consolePrev;
}

void Log::trace(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Trace, fmt, ap);
	va_end(ap);
}

void Log::debug(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Debug, fmt, ap);
	va_end(ap);
}

void Log::info(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Info, fmt, ap);
	va_end(ap);
}

void Log::warn(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Warn, fmt, ap);
	va_end(ap);
}

void Log::error(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Error, fmt, ap);
	va_end(ap);
}

void Log::crit(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Crit, fmt, ap);
	va_end(ap);
}

void Log::fail(char const* const fmt, ...)
{
	va_list ap = {};
	va_start(ap, fmt);
	logPrio(*this, Level::Crit, fmt, ap);
	va_end(ap);
#ifdef __GNUC__
	__builtin_trap();
#endif //__GNUC__
	std::exit(EXIT_FAILURE);
}

Log::~Log()
{
#ifndef NDEBUG
	if (file) {
		console = true;
		warn("Logfile was never closed");
	}
#endif //NDEBUG
}

}
