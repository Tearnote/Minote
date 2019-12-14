// Minote - log.tcc

#ifndef MINOTE_LOG_TCC
#define MINOTE_LOG_TCC

#include "log.h"

#include <tuple>
#include "gsl/gsl"
#include "asap/asap.h"
#include "fmt/format.h"
#include "fmt/ranges.h"

template<typename ...Args>
auto Log::log(const Level lv, std::string_view str, Args... args) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};

	Expects(targets[File] == (logFile != nullptr));

	if (lv < level)
		return;

	if (targets[Console]) {
		if (lv < Warn)
			logTo(stdout, lv, str, args...);
		else
			logTo(stderr, lv, str, args...);
	}

	if (targets[File])
		logTo(logFile, lv, str, args...);
}

template<typename ...Args>
auto Log::logTo(FILE* out, const Log::Level lv, std::string_view str, Args... args) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};

	fmt::print(out, "{} [{}] {}\n", asap::now().str(), levelNames[lv], fmt::join(std::tuple{str, args...}, ""));
#if !NDEBUG
	std::fflush(out);
#endif //!NDEBUG
}

#endif //MINOTE_LOG_TCC
