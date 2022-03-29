#include "base/log.hpp"

#include <string> // std::string needed by quill API
#include "quill/Quill.h"
#include "base/containers/string.hpp"
#include "base/containers/array.hpp"
#include "base/util.hpp"

namespace minote::base {

using namespace literals;

void Log::init(string_view _filename, quill::LogLevel _level) {
	
	quill::enable_console_colours();
	quill::start(true);
	
	auto file = quill::file_handler(std::string(_filename), "w");
	auto console = quill::stdout_handler();
	
	file->set_pattern(
		QUILL_STRING("%(ascii_time) [%(level_name)] %(message)"),
		"%H:%M:%S.%Qns",
		quill::Timezone::LocalTime);
	console->set_pattern(
		QUILL_STRING("%(ascii_time) %(message)"),
		"%H:%M:%S",
		quill::Timezone::LocalTime);
	
	m_logger = quill::create_logger("main", {file, console});
	m_logger->set_log_level(_level);
	
}

}
