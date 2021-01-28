#include "game.hpp"

#include <exception>
#include "base/log.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
#include "playstate.hpp"
#include "main.hpp"

namespace minote {

using namespace base;

constexpr auto UpdateTick = 1_s / 120;

void game(sys::Glfw& glfw, sys::Window& window) try {

	// *** Initialization ***

	engine::Mapper mapper;
	auto engine = gfx::Engine{glfw, window, AppTitle, AppVersion};

	PlayState play;

	// *** Main loop ***

	auto nextUpdate = sys::Glfw::getTime();

	while (!window.isClosing()) {
		// Input
		mapper.collectKeyInputs(window);

		// Logic
		while (nextUpdate <= sys::Glfw::getTime()) {
			svector<engine::Mapper::Action, 64> updateActions;

			mapper.processActions([&](auto const& action) {
				if (action.timestamp > nextUpdate) return false;

				updateActions.push_back(action);

				// Interpret quit events here for now
				using Action = engine::Mapper::Action::Type;
				if (action.type == Action::Back)
					window.requestClose();

				return true;
			});

			play.tick(updateActions);
			nextUpdate += UpdateTick;
		}

		// Graphics
		engine.setBackground({0.75f, 0.75f, 0.75f});
		engine.setLightSource({-8.0f, 32.0f, 16.0f}, {1.0f, 1.0f, 1.0f});
		engine.setCamera({0.0f, 10.0f, 24.0f}, {0.0f, 10.0f, 0.0f});

		play.draw(engine);

		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
