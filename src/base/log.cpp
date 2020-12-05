#include "log.hpp"

#include "base/util.hpp"
#include "base/io.hpp"

namespace minote {

void Log::enableFile(file&& _logfile) {
	ASSERT(!logfile);
	ASSERT(_logfile);

	logfile = move(_logfile);
}

void Log::disableFile() try {
	logfile.close();
} catch (system_error const& e) {
	print(stderr, R"(Could not close logfile "{}": {})", logfile.where(), e.what());
}

}
