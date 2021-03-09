#include "game.hpp"

#include <exception>
#include "base/svector.hpp"
#include "base/log.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
//#include "playstate.hpp"
#include "main.hpp"

namespace minote {

using namespace base;

constexpr auto UpdateTick = 1_s / 120;

void game(sys::Glfw& glfw, sys::Window& window) try {

	// *** Initialization ***

	Mapper mapper;
	auto engine = gfx::Engine(window);
	engine.setup();

//	PlayState play;

	// *** Main loop ***

	auto nextUpdate = sys::Glfw::getTime();

	auto lightSource = glm::vec3{6.0f, 12.0f, -6.0f};
	auto held = std::array<bool, 4>{};

	while (!window.isClosing()) {
		// Input
		mapper.collectKeyInputs(window);

		// Logic
		while (nextUpdate <= sys::Glfw::getTime()) {
			svector<Mapper::Action, 64> updateActions;

			mapper.processActions([&](auto const& action) {
				if (action.timestamp > nextUpdate) return false;

				updateActions.push_back(action);

				// Interpret quit events here for now
				using Action = Mapper::Action::Type;
				if (action.type == Action::Back)
					window.requestClose();

				// Quick and dirty camera control
				if (action.type == Action::Left)
					held[0] = action.state == Mapper::Action::State::Pressed;
				if (action.type == Action::Right)
					held[1] = action.state == Mapper::Action::State::Pressed;
				if (action.type == Action::Drop)
					held[2] = action.state == Mapper::Action::State::Pressed;
				if (action.type == Action::Lock)
					held[3] = action.state == Mapper::Action::State::Pressed;

				return true;
			});
			if (held[0])
				lightSource.x -= 0.1f;
			if (held[1])
				lightSource.x += 0.1f;
			if (held[2])
				lightSource.z -= 0.1f;
			if (held[3])
				lightSource.z += 0.1f;

//			play.tick(updateActions);
			nextUpdate += UpdateTick;
		}

		// Graphics
		engine.setBackground({0.4f, 0.4f, 0.4f});
		engine.setLightSource(lightSource, {1.0f, 1.0f, 1.0f});
		engine.setCamera({0.0f, 12.0f, 24.0f}, {0.0f, 4.0f, 0.0f});

		auto const centerTransform = make_translate({-0.5f, -0.5f, -0.5f});
		auto const rotateTransform = make_rotate(glm::radians(-90.0f), {1.0f, 0.0f, 0.0f});
		auto const rotateTransformAnim = make_rotate(f32(glm::radians(f64(sys::Glfw::getTime().count()) / 20000000.0)), {0.0f, 1.0f, 0.0f});

		engine.enqueue("block"_id, std::array{
			gfx::Engine::Instance{
				.transform = make_translate({0.0f, -1.0f, 0.0f}) * make_scale({16.0f, 2.0f, 16.0f}) * rotateTransform * centerTransform,
				.tint = {0.9f, 0.9f, 1.0f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({-4.0f, 1.0f, -4.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({4.0f, 1.0f, -4.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({-4.0f, 1.0f, 4.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({4.0f, 1.0f, 4.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.9f, 0.1f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({2.0f, 1.0f, 0.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.1f, 0.5f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({2.0f, 2.75f, 0.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.1f, 0.7f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({2.0f, 4.5f, 0.0f}) * make_scale({2.0f, 2.0f, 2.0f}) * rotateTransform * centerTransform,
				.tint = {0.1f, 0.9f, 0.1f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
			gfx::Engine::Instance{
				.transform = make_translate({-2.0f, 1.5f, 0.0f}) * make_scale({3.0f, 3.0f, 3.0f}) * rotateTransformAnim * rotateTransform * centerTransform,
				.tint = {0.2f, 0.9f, 0.5f, 1.0f},
				.ambient = 0.1f,
				.diffuse = 1.0f,
				.specular = 0.4f,
				.shine = 24.0f,
			},
		});

		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
