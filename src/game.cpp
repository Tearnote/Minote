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

void queueScene(Engine& engine) {
	engine.setBackground(glm::vec3{0.8f, 0.8f, 0.8f});
	engine.setLightSource(glm::vec3{-4.0f, 8.0f, -4.0f}, glm::vec3{1.0f, 1.0f, 1.0f});
	engine.setCamera(glm::vec3{0.0f, 2.0f, -2.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

	auto const modelTranslation = make_translate(glm::vec3{-0.5f, -0.5f, -0.5f});
	auto const modelRotation = make_rotate(glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
	auto const modelSpin = make_rotate(ratio(Glfw::getTime(), 1_s), glm::vec3{0.0f, 1.0f, 0.0f});
	auto const modelTranslationBack = make_translate(glm::vec3{0.0f, 0.0f, 0.5f});
	auto const model = modelTranslationBack * modelSpin * modelRotation * modelTranslation;

	auto const modelTranslation2 = make_translate(glm::vec3{-1.5f, 0.0f, 0.0f});
	auto const modelScale = make_scale(glm::vec3{0.1f, 0.1f, 0.1f});
	auto const model2 = modelTranslation2 * modelSpin * modelScale;

	auto const modelTranslation3 = make_translate(glm::vec3{1.5f, 0.0f, 0.0f});
	auto const model3 = modelTranslation3 * modelSpin * modelScale;

	engine.enqueueDraw("block"_id, "opaque"_id,
		std::to_array<Instance>({
			{.transform = model},
		}),
		Material::Phong, MaterialData{.phong = {
			.ambient = 0.2f,
			.diffuse = 0.9f,
			.specular = 0.4f,
			.shine = 24.0f,
		}}
	);
	engine.enqueueDraw("scene"_id, "transparent"_id,
		std::to_array<Instance>({
			{.transform = model2, .tint = {1.0f, 0.0f, 0.0f, 1.0f}},
			{.transform = model3},
		}),
		Material::Flat
	);
}

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
		queueScene(engine);
		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
