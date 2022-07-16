#include "util/log.hpp"

#include <string> // std::string needed by quill API
#include "quill/Quill.h"
#include "util/string.hpp"
#include "util/array.hpp"
#include "util/util.hpp"

namespace minote {

void Log::init(string_view _filename, quill::LogLevel _level) {
	
	auto file = quill::file_handler(std::string(_filename), "w");
	auto console = quill::stdout_handler();
	static_cast<quill::ConsoleHandler*>(console)->enable_console_colours();
	
	file->set_pattern(
		"%(ascii_time) [%(level_name)] %(message)",
		"%H:%M:%S.%Qns",
		quill::Timezone::LocalTime);
	console->set_pattern(
		"%(ascii_time) %(message)",
		"%H:%M:%S",
		quill::Timezone::LocalTime);
	
	
	auto cfg = quill::Config();
	cfg.default_handlers.emplace_back(file);
	cfg.default_handlers.emplace_back(console);
	quill::configure(cfg);
	quill::start(true);
	
	m_logger = quill::get_logger();
	m_logger->set_log_level(_level);
	
}

}
