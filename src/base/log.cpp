#include "log.hpp"

#include "base/util.hpp"

namespace minote {

void Log::enableFile(fs::path const& filepath)
{
	if (file) {
		warn("Not opening logfile at {}: already logging to a file", filepath.string());
		return;
	}

	file = std::fopen(filepath.string().c_str(), "w");
	if (!file) {
		fmt::print(stderr, "Failed to open logfile at {} for writing: {}\n",
			filepath.string(), std::strerror(errno));
		errno = 0;
	}
}

void Log::disableFile()
{
	if (!file) return;

	auto const result = std::fclose(file);
	file = nullptr;
	if (result) {
		std::perror("Failed to flush logfile");
		errno = 0;
	}
}

Log::~Log()
{
	disableFile();
}

}
