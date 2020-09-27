/**
 * Implementation of log.hpp
 * @file
 */

#include "log.hpp"

#include <cstdarg>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <ctime>
#include "util.hpp"

namespace minote::log {

/// Mapping from Log::Level to string name
static constexpr char logLevelStrings[][+Log::Level::Size]{
	"", "TRACE", "DEBUG", " INFO", " WARN", "ERROR", " CRIT"
};

/**
 * Write a log message to a specified output. Attaches a timestamp and formats
 * the log level.
 * @param file The output file
 * @param level Message level
 * @param fmt Format string in printf syntax
 * @param ap List of arguments to print
 */
static void logTo(FILE* const file, const Log::Level level,
	const char* const fmt, va_list& ap)
{
	const auto printTimestamp{[=]() {
		const auto now{std::time(nullptr)};
		// Copy-initialized to minimize chance of thread safety issues
		const auto localnow{*std::localtime(&now)};
		return std::fprintf(file, "%02d:%02d:%02d [%s] ",
			localnow.tm_hour, localnow.tm_min, localnow.tm_sec,
			logLevelStrings[+level]) < 0;
	}};
	const auto printMessage{[=]() {
		return std::vfprintf(file, fmt, ap) < 0;
	}};
	const auto printNewline{[=]() {
		return std::fputc('\n', file) == EOF;
	}};

	if (printTimestamp() || printMessage() || printNewline()) {
		std::perror("Failed to write into logfile");
		errno = 0;
	}
}

/**
 * The actual log message handling function. Performs level filtering and
 * output selection.
 * @param log The ::Log object
 * @param level Message level
 * @param fmt Format string in printf syntax
 * @param ap List of arguments to print
 */
static void logPrio(Log& log, const Log::Level level,
	const char* const fmt, va_list& ap)
{
	assert(fmt);
	assert(ap);
	using Level = Log::Level;

	if (level < log.level)
		return;

	if (log.console) {
		FILE* const output{[=]() {
			if (level >= Level::Warn)
				return stderr;
			else
				return stdout;
		}()};
		va_list apcopy{};
		va_copy(apcopy, ap);
		logTo(output, level, fmt, apcopy);
	}
	if (log.file) {
		va_list apcopy{};
		va_copy(apcopy, ap);
		logTo(log.file, level, fmt, apcopy);
	}
}

Log::~Log()
{
	if (!file)
		return;

	console = true;
	warn("Logfile was not closed on object destruction");
}

void Log::enableFile(const char* const filepath)
{
	assert(filepath);
	if (file) {
		warn("Not opening logfile %s: already logging to a file", filepath);
		return;
	}

	file = std::fopen(filepath, "w");
	if (file)
		return;

	// Handle error by forcibly printing it to console
	const auto consolePrev = console;
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

	const auto result = fclose(file);
	file = nullptr;
	if (!result)
		return;

	// Handle error by forcibly printing it to console
	const auto consolePrev = console;
	console = true;
	error("Failed to flush logfile: %s", std::strerror(errno));
	errno = 0;
	console = consolePrev;
}

void Log::trace(const char* const fmt, ...)
{
	va_list ap{};
	va_start(ap, fmt);
	logPrio(*this, Level::Trace, fmt, ap);
	va_end(ap);
}

void Log::debug(const char* const fmt, ...)
{
	va_list ap{};
	va_start(ap, fmt);
	logPrio(*this, Level::Debug, fmt, ap);
	va_end(ap);
}

void Log::info(const char* const fmt, ...)
{
	va_list ap{};
	va_start(ap, fmt);
	logPrio(*this, Level::Info, fmt, ap);
	va_end(ap);
}

void Log::warn(const char* const fmt, ...)
{
	va_list ap{};
	va_start(ap, fmt);
	logPrio(*this, Level::Warn, fmt, ap);
	va_end(ap);
}

void Log::error(const char* const fmt, ...)
{
	va_list ap{};
	va_start(ap, fmt);
	logPrio(*this, Level::Error, fmt, ap);
	va_end(ap);
}

void Log::crit(const char* const fmt, ...)
{
	va_list ap{};
	va_start(ap, fmt);
	logPrio(*this, Level::Crit, fmt, ap);
	va_end(ap);
}

}
