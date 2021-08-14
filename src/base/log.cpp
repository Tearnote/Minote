#include "base/log.hpp"

#include <string> // std::string needed by quill API
#include "quill/Quill.h"
#include "Tracy.hpp"
#include "base/containers/string.hpp"
#include "base/containers/array.hpp"
#include "base/util.hpp"

namespace minote::base {

using namespace literals;

// Internal glue to report log messages to the profiler
struct TracyHandler : public quill::Handler {
	
	void write(
		fmt::memory_buffer const& record,
		std::chrono::nanoseconds timestamp,
		quill::LogLevel severity) override;
	
	void flush() noexcept override {}
	
private:
	
	static constexpr auto m_colors = to_array<u32>({
		tracy::Color::White,          // TraceL3
		tracy::Color::White,          // TraceL2
		tracy::Color::White,          // TraceL1
		tracy::Color::LightSkyBlue,   // Debug
		tracy::Color::MediumSeaGreen, // Info
		tracy::Color::Gold,           // Warning
		tracy::Color::IndianRed,      // Error
		tracy::Color::Purple,         // Critical
	});
	
};

void Log::init(string_view _filename, quill::LogLevel _level) {
	
	quill::enable_console_colours();
	quill::start(true);
	
	auto file = quill::file_handler(std::string(_filename), "w");
	auto console = quill::stdout_handler();
	auto tracy = quill::create_handler<TracyHandler>("tracy");
	
	file->set_pattern(
		QUILL_STRING("%(ascii_time) [%(level_name)] %(message)"),
		"%H:%M:%S.%Qns",
		quill::Timezone::LocalTime);
	console->set_pattern(
		QUILL_STRING("%(ascii_time) %(message)"),
		"%H:%M:%S",
		quill::Timezone::LocalTime);
	tracy->set_pattern(QUILL_STRING("%(message)"));
	
	m_logger = quill::create_logger("main", {file, console, tracy});
	m_logger->set_log_level(_level);
	
}

void TracyHandler::write(
		fmt::memory_buffer const& _record,
		std::chrono::nanoseconds, quill::LogLevel _severity) {
	
	TracyMessageC(_record.data(), _record.size(), m_colors[+_severity]);
	
}

}
