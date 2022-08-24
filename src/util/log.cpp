#include "util/log.hpp"

#include "fmtlog.h"
#include "util/string.hpp"

namespace minote {

void Log::init(string_view _filename/*, quill::LogLevel _level*/) {
	
	//fmtlog::setLogFile(string(_filename).c_str(), false);
	fmtlog::startPollingThread(1);
	/*
	file->set_pattern(
		"%(ascii_time) [%(level_name)] %(message)",
		"%H:%M:%S.%Qns",
		quill::Timezone::LocalTime);
	console->set_pattern(
		"%(ascii_time) %(message)",
		"%H:%M:%S",
		quill::Timezone::LocalTime);
	
	m_logger->set_log_level(_level);
	*/
}

}
