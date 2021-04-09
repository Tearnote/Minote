#include "game.hpp"

#include "config.hpp"

#include <exception>
#include <stdexcept>
#include <vector>
#include <memory>
#include <span>
#include "sqlite3.h"
#include "base/math.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
//#include "playstate.hpp"
#include "config.hpp"
#include "main.hpp"

#include "GLFW/glfw3.h"
#include "imgui.h"

namespace minote {

using namespace base;

constexpr auto UpdateTick = 1_s / 120;

void game(sys::Glfw&, sys::Window& window) try {

	// *** Initialization ***

	auto mapper = Mapper();
	auto engine = gfx::Engine(window, AppVersion);

	{
		auto* assets = (sqlite3*)(nullptr);
		if (auto result = sqlite3_open_v2(AssetsPath, &assets, SQLITE_OPEN_READONLY, nullptr); result != SQLITE_OK) {
			sqlite3_close(assets);
			throw std::runtime_error(fmt::format(R"(Failed to open database "{}": {})", AssetsPath, sqlite3_errstr(result)));
		}
		defer {
			if (auto result = sqlite3_close(assets); result != SQLITE_OK)
				L.warn(R"(Failed to close database "{}": {})", AssetsPath, sqlite3_errstr(result));
		};

		{
			auto modelsQuery = (sqlite3_stmt*)(nullptr);
			if (auto result = sqlite3_prepare_v2(assets, "SELECT * from models", -1, &modelsQuery, nullptr); result != SQLITE_OK)
				throw std::runtime_error(fmt::format(R"(Failed to query database "{}": {})", AssetsPath, sqlite3_errstr(result)));
			defer { sqlite3_finalize(modelsQuery); };
			if (sqlite3_column_count(modelsQuery) != 2)
				throw std::runtime_error(fmt::format(R"(Invalid number of columns in table "models" in database "{}")", AssetsPath));

			auto result = SQLITE_OK;
			while (result = sqlite3_step(modelsQuery), result != SQLITE_DONE) {
				if (result != SQLITE_ROW)
					throw std::runtime_error(fmt::format(R"(Failed to query database "{}": {})", AssetsPath, sqlite3_errstr(result)));
				if (sqlite3_column_type(modelsQuery, 0) != SQLITE_TEXT)
					throw std::runtime_error(fmt::format(R"(Invalid type in column 0 of table "models" in database "{}")", AssetsPath));
				if (sqlite3_column_type(modelsQuery, 1) != SQLITE_BLOB)
					throw std::runtime_error(fmt::format(R"(Invalid type in column 1 of table "models" in database "{}")", AssetsPath));

				auto name = (char const*)(sqlite3_column_text(modelsQuery, 0));
				auto nameLen = sqlite3_column_bytes(modelsQuery, 0);
				auto model = (char const*)(sqlite3_column_blob(modelsQuery, 1));
				auto modelLen = sqlite3_column_bytes(modelsQuery, 1);
				engine.addModel(std::string_view(name, nameLen), std::span(model, modelLen));
			}
		}
	}
	engine.setup();

//	PlayState play;

	// *** Main loop ***

	auto nextUpdate = sys::Glfw::getTime();

	auto Expand = 0;
	while (!window.isClosing()) {
		// Input
		mapper.collectKeyInputs(window);

		// Logic
		auto updateActions = std::vector<Mapper::Action>();
		updateActions.reserve(16);
		while (nextUpdate <= sys::Glfw::getTime()) {
			updateActions.clear();
			mapper.processActions([&](auto const& action) {
				if (action.timestamp > nextUpdate) return false;

#if IMGUI
				if (action.state == Mapper::Action::State::Pressed && ImGui::GetIO().WantCaptureKeyboard)
					return false;
#endif //IMGUI

				updateActions.push_back(action);

				// Interpret quit events here for now
				using Action = Mapper::Action::Type;
				if (action.type == Action::Back)
					window.requestClose();

				return true;
			});

//			play.tick(updateActions);
			nextUpdate += UpdateTick;
		}

		// Graphics
		engine.setCamera({std::sin(glfwGetTime() / 4.0) * 28.0f, std::sin(glfwGetTime() / 3.1) * 4.0f + 12.0f, std::cos(glfwGetTime() / 4.0) * 28.0f}, {0.0f, 4.0f, 0.0f});

		ImGui::SliderInt("Expand", &Expand, 0, 30);
		constexpr auto Spacing = 25.0f;
		for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
		for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
			auto offset = make_translate({x, 0.0f, y});
			auto rotateTransform = make_rotate(glm::radians(90.0f), {1.0f, 0.0f, 0.0f});
			auto rotateTransformAnim = make_rotate(f32(glm::radians(f64(sys::Glfw::getTime().count()) / 20000000.0)), {0.0f, 1.0f, 0.0f});

			engine.enqueue("block"_id, std::array{
				gfx::Engine::Instance{
					.transform = offset * make_translate({0.0f, 0.0f, 0.0f}) * make_scale({12.0f, 1.0f, 12.0f}) * glm::inverse(rotateTransform),
					.tint = {0.9f, 0.9f, 1.0f, 1.0f},
				},
				gfx::Engine::Instance{
					.transform = offset * make_translate({-4.0f, 2.0f, -4.0f}) * rotateTransform,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				},
				gfx::Engine::Instance{
					.transform = offset * make_translate({4.0f, 2.0f, -4.0f}) * rotateTransform,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				},
				gfx::Engine::Instance{
					.transform = offset * make_translate({-4.0f, 2.0f, 4.0f}) * rotateTransform,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				},
				gfx::Engine::Instance{
					.transform = offset * make_translate({4.0f, 2.0f, 4.0f}) * rotateTransform,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				},
				gfx::Engine::Instance{
					.transform = offset * make_translate({0.0f, 2.5f, 0.0f}) * make_scale({1.5f, 1.5f, 1.5f}) * rotateTransformAnim * rotateTransform,
					.tint = {0.2f, 0.9f, 0.5f, 1.0f},
					.roughness = 0.2f,
					.metalness = 0.9f,
				},
			});
			for (auto i = 0.0f; i <= 1.0f; i += 0.125f)
				engine.enqueue("sphere"_id, std::array{gfx::Engine::Instance{
					.transform = offset * make_translate({(i - 0.5f) * 2.0f * 8.0f, 2.0f, 8.0f}),
					.roughness = i,
					.metalness = 0.9f,
				}});
			for (auto i = 0.0f; i <= 1.0f; i += 0.125f)
				engine.enqueue("sphere"_id, std::array{gfx::Engine::Instance{
					.transform = offset * make_translate({(i - 0.5f) * 2.0f * 8.0f, 2.0f, -8.0f}),
					.roughness = i,
					.metalness = 0.1f,
				}});
		}

#if IMGUI
		ImGui::ShowDemoWindow();
#endif //IMGUI
		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
