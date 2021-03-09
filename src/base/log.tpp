#pragma once

#include <system_error>
#include <string_view>
#include <utility>
#include <array>
#include <ctime>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include "base/file.hpp"
#include "base/util.hpp"

namespace minote::base {

using namespace std::string_view_literals;
using namespace base::literals;

namespace detail {

// Mapping from Log::Level to string name. Keep aligned to 5 chars
constexpr auto LogLevelStrings = std::array{
	""sv, "TRACE"sv, "DEBUG"sv, " INFO"sv, " WARN"sv, "ERROR"sv, " CRIT"sv
};

// Write a preformatted log message to a specified output. Does not insert
// a newline.
inline void logTo(file& f, std::string_view msg) try {
	fmt::print(f, msg);
} catch (std::system_error const& e) {
	fmt::print(cerr, R"(Failed to write to logfile "{}": {}\n)", f.where(), e.what());
}

}

template<typename S, typename... Args>
void Log::trace(S const& fmt, Args&& ... args) {
	log(Level::Trace, fmt, std::forward<Args>(args)...);
}

template<typename S, typename... Args>
void Log::debug(S const& fmt, Args&& ... args) {
	log(Level::Debug, fmt, std::forward<Args>(args)...);
}

template<typename S, typename... Args>
void Log::info(S const& fmt, Args&& ... args) {
	log(Level::Info, fmt, std::forward<Args>(args)...);
}

template<typename S, typename... Args>
void Log::warn(S const& fmt, Args&& ... args) {
	log(Level::Warn, fmt, std::forward<Args>(args)...);
}

template<typename S, typename... Args>
void Log::error(S const& fmt, Args&& ... args) {
	log(Level::Error, fmt, std::forward<Args>(args)...);
}

template<typename S, typename... Args>
void Log::crit(S const& fmt, Args&& ... args) {
	log(Level::Crit, fmt, std::forward<Args>(args)...);
}

template<typename S, typename... Args>
void Log::log(Log::Level const _level, S const& fmt, Args&&... args) {
	if (_level < level) return;
	if (!console && !logfile) return;

	fmt::memory_buffer msg;

	// Insert timestamp and level
	auto const now = std::time(nullptr);
	auto const localnow = *std::localtime(&now);
	fmt::format_to(msg, "{:%H:%M:%S} [{}] ", localnow, detail::LogLevelStrings[+_level]);

	// Insert formatted message
	fmt::format_to(msg, fmt, std::forward<Args>(args)...);
	fmt::format_to(msg, "\n\0"sv); // Finalize

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
