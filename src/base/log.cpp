#include "log.hpp"

#include "base/util.hpp"

namespace minote {

void Log::enableFile(path const& filepath)
{
	if (file) {
		warn("Not opening logfile %s: already logging to a file", filepath.string());
		return;
	}

	file = std::fopen(filepath.string().c_str(), "w");
	if (file) return;

	// Handle error by forcibly printing it to console
	auto const consolePrev = console;
	console = true;
	error("Failed to open logfile %s for writing: %s", filepath.string(), std::strerror(errno));
	errno = 0;
	console = consolePrev;
}

void Log::disableFile()
{
	if (!file) return;

	auto const result = std::fclose(file);
	file = nullptr;
	if (!result) return;

	// Handle error by forcibly printing it to console
	auto const consolePrev = console;
	console = true;
	error("Failed to flush logfile: %s", std::strerror(errno));
	errno = 0;
	console = consolePrev;
}

Log::~Log()
{
	disableFile();
}

}
