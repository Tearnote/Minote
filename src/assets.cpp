#include "assets.hpp"

#include <stdexcept>
#include "quill/Fmt.h"
#include "base/log.hpp"

namespace minote {

using namespace base;

Assets::Assets(std::string_view _path) {
	path = std::string(_path);
	if (auto result = sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr); result != SQLITE_OK) {
		sqlite3_close(db);
		throw std::runtime_error(fmt::format(R"(Failed to open database "{}": {})", path, sqlite3_errstr(result)));
	}
}

Assets::~Assets() {
	if (auto result = sqlite3_close(db); result != SQLITE_OK)
		L_WARN(R"(Failed to close database "{}": {})", path, sqlite3_errstr(result));
}

}
