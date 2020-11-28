#include "log.hpp"

#include <cstring>
#include <cstdio>
#include "base/util.hpp"

namespace minote {

void Log::enableFile(char const* const filepath)
{
	ASSERT(filepath);
	if (file) {
		warn("Not opening logfile %s: already logging to a file", filepath);
		return;
	}

	file = std::fopen(filepath, "w");
	if (file) return;

	// Handle error by forcibly printing it to console
	auto const consolePrev = console;
	console = true;
	error("Failed to open logfile %s for writing: %s",
		filepath, std::strerror(errno));
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
