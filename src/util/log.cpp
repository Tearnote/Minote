#include "util/log.hpp"

#include <array>
#include "fmtlog.h"
#include "util/string.hpp"

namespace minote {

void Log::init(string_view _filename, fmtlog::LogLevel _level) {
	
	fmtlog::setLogFile(string(_filename).c_str(), true);
	fmtlog::setLogLevel(_level);
	fmtlog::flushOn(fmtlog::WRN);
	fmtlog::setLogCB(&consoleCallback, _level);
	fmtlog::setHeaderPattern("{HMSe} [{l}] ");
	fmtlog::startPollingThread(1);

}

void Log::consoleCallback(int64, fmtlog::LogLevel _level, fmt::string_view, usize,
	fmt::string_view, fmt::string_view _msg, usize, usize) {

	constexpr static auto Colors = std::to_array({
		"\x1B[32m", // DBG
		"\x1B[36m", // INF
		"\x1B[33m", // WRN
		"\x1B[31m", // ERR
		"\033[0m", // Clear
	});
	fmt::print("{}{}{}\n", Colors[_level], _msg, Colors.back());

}

}
