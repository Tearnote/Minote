#pragma once

#include <stdexcept>
#include "fmt/core.h"
#include "base/util.hpp"

namespace minote {

using namespace base;

template<typename F>
	requires std::invocable<F, std::string_view, std::span<char const>>
void Assets::loadModels(F func) {
	auto modelsQuery = (sqlite3_stmt*)(nullptr);
	if (auto result = sqlite3_prepare_v2(db, "SELECT * from models", -1, &modelsQuery, nullptr); result != SQLITE_OK)
		throw std::runtime_error(fmt::format(R"(Failed to query database "{}": {})", path, sqlite3_errstr(result)));
	defer { sqlite3_finalize(modelsQuery); };
	if (sqlite3_column_count(modelsQuery) != 2)
		throw std::runtime_error(fmt::format(R"(Invalid number of columns in table "models" in database "{}")", path));

	auto result = SQLITE_OK;
	while (result = sqlite3_step(modelsQuery), result != SQLITE_DONE) {
		if (result != SQLITE_ROW)
			throw std::runtime_error(fmt::format(R"(Failed to query database "{}": {})", path, sqlite3_errstr(result)));
		if (sqlite3_column_type(modelsQuery, 0) != SQLITE_TEXT)
			throw std::runtime_error(fmt::format(R"(Invalid type in column 0 of table "models" in database "{}")", path));
		if (sqlite3_column_type(modelsQuery, 1) != SQLITE_BLOB)
			throw std::runtime_error(fmt::format(R"(Invalid type in column 1 of table "models" in database "{}")", path));

		auto name = (char const*)(sqlite3_column_text(modelsQuery, 0));
		auto nameLen = sqlite3_column_bytes(modelsQuery, 0);
		auto model = (char const*)(sqlite3_column_blob(modelsQuery, 1));
		auto modelLen = sqlite3_column_bytes(modelsQuery, 1);
		func(std::string_view(name, nameLen), std::span(model, modelLen));
	}
}

}
