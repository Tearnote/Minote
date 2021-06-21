#pragma once

#include <string_view>
#include "quill/Quill.h"

namespace minote::base {

struct Log {
	
	inline static quill::Logger* logger;
	
	static void init(std::string_view filename, quill::LogLevel level);
	
};

#define L_TRACE(fmt, ...) LOG_TRACE_L1(Log::logger, fmt, ##__VA_ARGS__)
#define L_DEBUG(fmt, ...) LOG_DEBUG(Log::logger, fmt, ##__VA_ARGS__)
#define L_INFO(fmt, ...) LOG_INFO(Log::logger, fmt, ##__VA_ARGS__)
#define L_WARN(fmt, ...) LOG_WARNING(Log::logger, fmt, ##__VA_ARGS__)
#define L_ERROR(fmt, ...) LOG_ERROR(Log::logger, fmt, ##__VA_ARGS__)
#define L_CRIT(fmt, ...) LOG_CRITICAL(Log::logger, fmt, ##__VA_ARGS__)

}
