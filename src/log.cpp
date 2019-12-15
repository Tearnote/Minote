// Minote - log.cpp

#include "log.h"

#include "fmt/core.h"

using namespace std::string_literals;

auto Log::setLevel(Log::Level lv) -> void
{
	std::unique_lock lock{mutex};
	level = lv;
}

auto Log::enable(const Log::Target tgt) -> void
{
	std::unique_lock lock{mutex};

	targets[tgt] = true;

	if (tgt == File) {
		logFile = std::fopen(logFilename.c_str(), "w");
		if (!logFile) {
			targets[File] = false; // Disable broken target
			const bool consoleState{targets[Console]};
			targets[Console] = true; // Temporarily enable console
			warn("Unable to open "s, logFilename, " for writing: ", std::string_view{strerror(errno)});
			targets[Console] = consoleState; // Switch to console's previous state
		}
	}

	Ensures(targets[File] == (logFile != nullptr));
}

auto Log::disable(const Log::Target tgt) -> void
{
	std::unique_lock lock{mutex};

	targets[tgt] = false;

	if (tgt == File && logFile) {
		std::fclose(logFile);
		logFile = nullptr;
	}

	Ensures(targets[File] == (logFile != nullptr));
}
