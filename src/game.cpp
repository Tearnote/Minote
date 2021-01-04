#include "game.hpp"

#include <stdexcept>
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "engine/engine.hpp"
#include "main.hpp"

namespace minote {

using namespace engine;
using namespace base;
using namespace sys;
using namespace vk;

void game(Glfw& glfw, Window& window) try {

	// *** Initialization ***

	auto engine = Engine{glfw, window, AppTitle, AppVersion};

	// *** Main loop ***

	while (!window.isClosing()) {
		// Input
		while (auto const input = window.getInput()) {
			if (input->keycode == Keycode::Escape)
				window.requestClose();
			window.popInput();
		}

		// Graphics
		engine.queueScene();
		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
