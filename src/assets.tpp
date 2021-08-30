#include "Tracy.hpp"
#include "base/error.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

namespace minote {

using namespace base;

template<typename F>
requires std::invocable<F, string_view, std::span<char const>>
void Assets::loadModels(F _func) {
	
	ZoneScoped;
	
	auto modelsQueryStr = fmt::format("SELECT * FROM {}", Models_n);
	
	auto modelsQuery = static_cast<sqlite3_stmt*>(nullptr);
	if (auto result = sqlite3_prepare_v2(m_db, modelsQueryStr.c_str(), -1, &modelsQuery, nullptr); result != SQLITE_OK)
		throw runtime_error_fmt("Failed to query database {}: {}", m_path, sqlite3_errstr(result));
	defer { sqlite3_finalize(modelsQuery); };
	if (sqlite3_column_count(modelsQuery) != 2)
		throw runtime_error_fmt("Invalid number of columns in table {} in database {}", Models_n, m_path);
	
	auto result = SQLITE_OK;
	while (result = sqlite3_step(modelsQuery), result != SQLITE_DONE) {
		
		if (result != SQLITE_ROW)
			throw runtime_error_fmt("Failed to query database {}: {}", m_path, sqlite3_errstr(result));
		if (sqlite3_column_type(modelsQuery, 0) != SQLITE_TEXT)
			throw runtime_error_fmt("Invalid type in column 0 of table models in database {}", m_path);
		if (sqlite3_column_type(modelsQuery, 1) != SQLITE_BLOB)
			throw runtime_error_fmt("Invalid type in column 1 of table models in database {}", m_path);
		
		auto name = reinterpret_cast<char const*>(sqlite3_column_text(modelsQuery, 0));
		auto nameLen = sqlite3_column_bytes(modelsQuery, 0);
		auto model = static_cast<char const*>(sqlite3_column_blob(modelsQuery, 1));
		auto modelLen = sqlite3_column_bytes(modelsQuery, 1);
		_func(string_view(name, nameLen), std::span(model, modelLen));
		
	}
	
	L_INFO("All models loaded from asset file {}", m_path);
	
}

}
