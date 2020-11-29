#include "log.hpp"

#include "base/io.hpp"

namespace minote {

void Log::enableFile(path const& filepath)
{
	if (file) {
		warn("Not opening logfile at {}: already logging to a file", filepath.string());
		return;
	}

	file = fopen(filepath.string().c_str(), "w");
	if (!file) {
		print(stderr, "Failed to open logfile at {} for writing: {}\n",
			filepath.string(), strerror(errno));
		errno = 0;
	}
}

void Log::disableFile()
{
	if (!file) return;

	auto const result = fclose(file);
	file = nullptr;
	if (result) {
		perror("Failed to flush logfile");
		errno = 0;
	}
}

Log::~Log()
{
	disableFile();
}

}
