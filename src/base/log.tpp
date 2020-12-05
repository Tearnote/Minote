#pragma once

#include "base/time_io.hpp"
#include "base/string.hpp"
#include "base/array.hpp"
#include "base/util.hpp"
#include "base/io.hpp"

namespace minote {

namespace detail {

// Mapping from Log::Level to string name. Keep aligned to 5 chars
constexpr auto LogLevelStrings = array{
	""sv, "TRACE"sv, "DEBUG"sv, " INFO"sv, " WARN"sv, "ERROR"sv, " CRIT"sv
};

// Write a preformatted log message to a specified output. Does not insert
// a newline.
inline void logTo(file& f, string_view msg) try
{
	print(f, msg);
} catch (system_error const& e) {
	print(cerr, R"(Failed to write to logfile "{}": {}\n)", f.where(), e.what());
}

}

template<typename S, typename... Args>
void Log::trace(S const& fmt, Args&& ... args)
{
	log(Level::Trace, fmt, args...);
}

template<typename S, typename... Args>
void Log::debug(S const& fmt, Args&& ... args)
{
	log(Level::Debug, fmt, args...);
}

template<typename S, typename... Args>
void Log::info(S const& fmt, Args&& ... args)
{
	log(Level::Info, fmt, args...);
}

template<typename S, typename... Args>
void Log::warn(S const& fmt, Args&& ... args)
{
	log(Level::Warn, fmt, args...);
}

template<typename S, typename... Args>
void Log::error(S const& fmt, Args&& ... args)
{
	log(Level::Error, fmt, args...);
}

template<typename S, typename... Args>
void Log::crit(S const& fmt, Args&& ... args)
{
	log(Level::Crit, fmt, args...);
}

template<typename S, typename... Args>
void Log::fail(S const& fmt, Args&& ... args)
{
	crit(fmt, args...);
#ifdef __GNUC__
	__builtin_trap();
#endif //__GNUC__
	exit(EXIT_FAILURE);
}

template<typename S, typename... Args>
void Log::log(Log::Level const _level, S const& fmt, Args&&... args)
{
	if (_level < level) return;
	if (!console && !logfile) return;

	memory_buffer msg;

	// Insert timestamp and level
	auto const now = time(nullptr);
	auto const localnow = *localtime(&now);
	format_to(msg, "{:%H:%M:%S} [{}] ", localnow, detail::LogLevelStrings[+_level]);

	// Insert formatted message
	format_to(msg, fmt, args...);
	format_to(msg, "\n\0"sv); // Finalize

	// Print constructed message to all enabled targets
	if (console) {
		if (level >= Level::Warn) {
			// Ensure previously written messages are not interleaved
			cout.flush();
			detail::logTo(cerr, msg.data());
			// Ensure message is written, in case the application crashes
			cerr.flush();
		} else {
			detail::logTo(cout, msg.data());
		}
	}

	if (logfile) {
		detail::logTo(logfile, msg.data());
		// Ensure message is written, in case the application crashes
		if (level >= Level::Warn)
			logfile.flush();
	}
}

}
