#pragma once

#include "config.hpp"

#include <format>
#include <span>
#include "sqlite3.h"
#include "stx/except.hpp"
#include "util/string.hpp"
#include "util/util.hpp"
#include "util/log.hpp"

namespace minote {

template<typename F>
requires std::invocable<F, string_view, std::span<char const>>
void Assets::loadModels(F _func) {
	
	// Prepare database query
	
	auto modelsQueryStr = std::format("SELECT * FROM {}", Models_table);
	
	auto modelsQuery = static_cast<sqlite3_stmt*>(nullptr);
	if (auto result = sqlite3_prepare_v2(m_db, modelsQueryStr.c_str(), -1, &modelsQuery, nullptr); result != SQLITE_OK)
		throw stx::runtime_error_fmt("Failed to query database {}: {}", m_path, sqlite3_errstr(result));
	defer { sqlite3_finalize(modelsQuery); };
	if (sqlite3_column_count(modelsQuery) != 2)
		throw stx::runtime_error_fmt("Invalid number of columns in table {} in database {}", Models_table, m_path);
	
	// Iterate over query results
	
	auto result = SQLITE_OK;
	while (result = sqlite3_step(modelsQuery), result != SQLITE_DONE) {
		
		if (result != SQLITE_ROW)
			throw stx::runtime_error_fmt("Failed to query database {}: {}", m_path, sqlite3_errstr(result));
		if (sqlite3_column_type(modelsQuery, 0) != SQLITE_TEXT)
			throw stx::runtime_error_fmt("Invalid type in column 0 of table models in database {}", m_path);
		if (sqlite3_column_type(modelsQuery, 1) != SQLITE_BLOB)
			throw stx::runtime_error_fmt("Invalid type in column 1 of table models in database {}", m_path);
		
		auto name = reinterpret_cast<char const*>(sqlite3_column_text(modelsQuery, 0));
		auto nameLen = sqlite3_column_bytes(modelsQuery, 0);
		auto model = static_cast<char const*>(sqlite3_column_blob(modelsQuery, 1));
		auto modelLen = sqlite3_column_bytes(modelsQuery, 1);
		_func(string_view(name, nameLen), std::span(model, modelLen));
		
	}
	
	L_INFO("All models loaded from asset file {}", m_path);
	
}

}
