#include "base/log.hpp"

#include <system_error>
#include <utility>

namespace minote::base {

void Log::enableFile(file&& _logfile) {
	if (logfile) disableFile();

	logfile = std::move(_logfile);
}

void Log::disableFile() try {
	logfile.close();
} catch (std::system_error const& e) {
	fmt::print(cerr, R"(Could not close logfile "{}": {})", logfile.where(), e.what());
}

}
