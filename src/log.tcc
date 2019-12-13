// Minote - log.tcc

#ifndef MINOTE_LOG_TCC
#define MINOTE_LOG_TCC

#include "log.h"

#include <iostream>
#include "gsl/gsl"
#include "asap/asap.h"

template<typename ...Args>
auto Log::log(const Level lv, std::string_view str, Args... args) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};

	Expects(targets[File] == logFile.is_open());

	if (lv < level)
		return;

	if (targets[Console]) {
		if (lv < Warn)
			logTo(std::cout, lv, str, args...);
		else
			logTo(std::cerr, lv, str, args...);
	}

	if (targets[File])
		logTo(logFile, lv, str, args...);
}

template<typename ...Args>
auto Log::logTo(std::ostream& out, const Log::Level lv, std::string_view str, Args... args) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};

	out << asap::now() << " [" << levelNames[lv] << "] " << str;
	(out << ... << args) << "\n";
#if !NDEBUG
	out.flush();
#endif //!NDEBUG
}

#endif //MINOTE_LOG_TCC
