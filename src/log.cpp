// Minote - log.cpp

#include "log.h"

#include <iostream>
#include <cstring>
#include <cerrno>
using namespace std::string_literals;

auto Log::setLevel(Log::Level lv) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};
	level = lv;
}

auto Log::enable(const Log::Target tgt) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};

	targets[tgt] = true;

	if (tgt == Console)
		std::ios_base::sync_with_stdio(false); // Speed up console streams

	if (tgt == File) {
		logFile.open(logFilename);
		if (!logFile.good()) {
			targets[File] = false; // Disable broken target
			const bool consoleState{targets[Console]};
			targets[Console] = true; // Temporarily enable console
			warn("Unable to open "s, logFilename, " for writing: ", strerror(errno));
			targets[Console] = consoleState; // Switch to console's previous state
		}
	}

	Ensures(targets[File] == logFile.is_open());
}

auto Log::disable(const Log::Target tgt) -> void
{
	std::unique_lock<std::recursive_mutex> lock{mutex};

	targets[tgt] = false;

	if (tgt == File && logFile.is_open())
		logFile.close();

	Ensures(targets[File] == logFile.is_open());
}
