// Minote - log.h

#ifndef MINOTE_LOG_H
#define MINOTE_LOG_H

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <mutex>
using namespace std::string_literals;

// Static logger class
// Prints timestamped messages to enabled targets
// Thread-safe
// Log levels available:
//   Trace - Any notable event. Very verbose
//   Debug - Information useful to the developer
//   Info  - Information useful to an inquisitive user
//   Warn  - Functionality is degraded
//   Error - Functionality is unavailable
//   Crit  - Cannot reasonably continue execution
class Log {
public:
	enum Target {
		Console = 0,
		File
	};
	enum Level {
		Trace = 0,
		Debug,
		Info,
		Warn,
		Error,
		Crit
	};

	// Set the most verbose level you want to log
	static auto setLevel(Level) -> void;

	// Control logging targets. All are disabled by default
	static auto enable(Target) -> void;
	static auto disable(Target) -> void;

	template<typename ...Args>
	static auto trace(std::string_view str, Args... args) { log(Trace, str, args...); }
	template<typename ...Args>
	static auto debug(std::string_view str, Args... args) { log(Debug, str, args...); }
	template<typename ...Args>
	static auto info(std::string_view str, Args... args) { log(Info, str, args...); }
	template<typename ...Args>
	static auto warn(std::string_view str, Args... args) { log(Warn, str, args...); }
	template<typename ...Args>
	static auto error(std::string_view str, Args... args) { log(Error, str, args...); }
	template<typename ...Args>
	static auto crit(std::string_view str, Args... args) { log(Crit, str, args...); }

	Log() = delete;

private:
	static inline std::recursive_mutex mutex{};
	static inline std::vector<bool> targets{false, false};
	static inline Level level{Debug};
	static inline const std::vector<std::string> levelNames{
			"TRACE"s,
			"DEBUG"s,
			"INFO"s,
			"WARN"s,
			"ERROR"s,
			"CRIT"s};
	static inline std::ofstream logFile{};
#if NDEBUG
	static inline const std::string logFilename{"minote.log"s};
#else //NDEBUG
	static inline const std::string logFilename{"minote-debug.log"s};
#endif //NDEBUG

	template<typename ...Args>
	static auto log(Level, std::string_view, Args...) -> void;

	template<typename ...Args>
	static auto logTo(std::ostream&, Level, std::string_view, Args...) -> void;
};

#include "log.tcc"

#endif //MINOTE_LOG_H
