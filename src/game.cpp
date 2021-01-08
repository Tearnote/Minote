#include "game.hpp"

#include <stdexcept>
#include "base/log.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "gfx/engine.hpp"
#include "engine/mapper.hpp"
#include "main.hpp"

namespace minote {

using namespace base;

void game(sys::Glfw& glfw, sys::Window& window) try {

	// *** Initialization ***

	engine::Mapper mapper;
	auto engine = gfx::Engine{glfw, window, AppTitle, AppVersion};

	// *** Main loop ***

	while (!window.isClosing()) {
		// Input
		mapper.collectKeyInputs(window);

		// Graphics
		engine.setBackground(glm::vec3{0.8f, 0.8f, 0.8f});
		engine.setLightSource(glm::vec3{-4.0f, 8.0f, -4.0f}, glm::vec3{1.0f, 1.0f, 1.0f});
		engine.setCamera(glm::vec3{0.0f, 2.0f, -2.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
