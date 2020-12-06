#include "log.hpp"

#include "base/io.hpp"

namespace minote {

void Log::enableFile(file&& _logfile) {
	if (logfile) disableFile();

	logfile = move(_logfile);
}

void Log::disableFile() try {
	logfile.close();
} catch (system_error const& e) {
	print(cerr, R"(Could not close logfile "{}": {})", logfile.where(), e.what());
}

auto assertHandler(char const* expr, char const* file, int line, char const* msg) -> int {
	L.crit(R"(Assertion "{}" triggered on line {} in {}{}{})",
		expr, line, file, msg? ": " : "", msg?: "");
	return true;
}

}
