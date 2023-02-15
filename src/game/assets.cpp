#include "game/assets.hpp"

#include "sqlite3.h"
#include "log.hpp"
#include "stx/except.hpp"

namespace minote::game {

Assets::Assets(std::string_view _path):
	m_path(_path) {
	
	if (auto result = sqlite3_open_v2(m_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr); result != SQLITE_OK) {
		sqlite3_close(m_db);
		throw stx::runtime_error_fmt("Failed to open database {}: {}", m_path, sqlite3_errstr(result));
	}
	
	L_INFO("Opened assets file {}", m_path);
	
}

Assets::~Assets() {
	
	if (auto result = sqlite3_close(m_db); result != SQLITE_OK)
		L_WARN("Failed to close database {}: {}", m_path, sqlite3_errstr(result));
	
}

}