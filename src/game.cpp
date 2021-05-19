#include "game.hpp"

#include "config.hpp"

#include <exception>
#include <vector>
#include "base/math.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "gfx/engine.hpp"
#include "assets.hpp"
#include "mapper.hpp"
//#include "playstate.hpp"
#include "config.hpp"
#include "main.hpp"

#include "GLFW/glfw3.h"
#include "imgui.h"

namespace minote {

using namespace base;
using namespace base::literals;

constexpr auto UpdateTick = 1_s / 120;

void game(sys::Glfw&, sys::Window& window) try {

	// *** Initialization ***

	auto mapper = Mapper();
	auto engine = gfx::Engine(window, AppVersion);

	auto assets = Assets(AssetsPath);
	assets.loadModels([&engine](auto name, auto data) {
		engine.addModel(name, data);
	});
	engine.uploadAssets();

	auto cameraPos = vec3(-10_m, -26_m, 10_m);
	auto cameraDir = vec3(0.0f, 1.0f, 0.0f);
	auto cameraUp = vec3(0.0f, 0.0f, 1.0f);
	auto camUp = false;
	auto camDown = false;
	auto camLeft = false;
	auto camRight = false;
	auto camFloat = false;
	auto const moveSpeed = 1_m / 16.0f;
	auto cursorLastPos = vec2();
	auto cursorOffset = vec2();
	auto lastButtonState = false;
	auto cameraYaw = 32_deg;
	auto cameraPitch = -12_deg;
	auto const lookSpeed = 1.0f / 256.0f;

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

				// Placeholder camera input
				auto state = action.state == Mapper::Action::State::Pressed? true : false;
				if (action.type == Action::Drop)
					camUp = state;
				if (action.type == Action::Lock)
					camDown = state;
				if (action.type == Action::Left)
					camLeft = state;
				if (action.type == Action::Right)
					camRight = state;
				if (action.type == Action::Skip)
					camFloat = state;

				return true;
			});

//			play.tick(updateActions);
			nextUpdate += UpdateTick;

			// Placeholder camera controls
			auto cursorNewPos = window.mousePos();
			if (!lastButtonState && window.mouseDown()) {
				lastButtonState = true;
				cursorLastPos = cursorNewPos;
			}
			if (!window.mouseDown()
#if IMGUI
				|| ImGui::GetIO().WantCaptureMouse
#endif //IMGUI
			)
				lastButtonState = false;
			if (lastButtonState) {
				cursorOffset += cursorNewPos - cursorLastPos;
				cursorLastPos = cursorNewPos;
			}

			cameraYaw += cursorOffset.x * lookSpeed;
			cameraPitch -= cursorOffset.y * lookSpeed;
			cameraPitch = clamp(cameraPitch, -89_deg, 89_deg);

			auto cameraTransform = make_rotate(cameraPitch, {1.0f, 0.0f, 0.0f});
			cameraTransform = make_rotate(cameraYaw, {0.0f, 0.0f, -1.0f}) * cameraTransform;
			cameraDir = cameraTransform * vec4(0.0f, 1.0f, 0.0f, 0.0f);
			cameraUp = cameraTransform * vec4(0.0f, 0.0f, 1.0f, 0.0f);

			if (camUp)
				cameraPos += cameraDir * moveSpeed;
			if (camDown)
				cameraPos -= cameraDir * moveSpeed;
			if (camLeft)
				cameraPos -= normalize(cross(cameraDir, cameraUp)) * moveSpeed;
			if (camRight)
				cameraPos += normalize(cross(cameraDir, cameraUp)) * moveSpeed;
			if (camFloat)
				cameraPos += vec3(0.0f, 0.0f, 1.0f) * moveSpeed;
		}

		// Graphics
		engine.setCamera(cameraPos, cameraPos + cameraDir);
		cursorOffset = vec2();

#if IMGUI
		ImGui::SliderInt("Expand", &Expand, 0, 40);
#endif //IMGUI

		auto prescale = make_scale({1_m, 1_m, 1_m});
		auto rotateTransform = make_rotate(180_deg, {1.0f, 0.0f, 0.0f});
		auto rotateTransformAnim = make_rotate(f32(radians(f64(sys::Glfw::getTime().count()) / 20000000.0)), {0.0f, 0.0f, 1.0f});
		auto transform1 = make_translate({0_m, 0_m, 0_m}) * make_scale({12.0f, 12.0f, 1.0f}) * inverse(rotateTransform);
		auto transform2 = make_translate({-4_m, -4_m, 2_m}) * rotateTransform;
		auto transform3 = make_translate({4_m, -4_m, 2_m}) * rotateTransform;
		auto transform4 = make_translate({-4_m, 4_m, 2_m}) * rotateTransform;
		auto transform5 = make_translate({4_m, 4_m, 2_m}) * rotateTransform;
		auto transform55 = make_translate({7_m, 0_m, 2_m}) * rotateTransform;
		auto transform6 = make_translate({0_m, 0_m, 2.5_m}) * make_scale({1.5f, 1.5f, 1.5f}) * rotateTransformAnim * rotateTransform;
		auto transform7 = make_translate({0_m, 8_m, 2_m});
		auto transform8 = make_translate({0_m, -8_m, 2_m});
		constexpr auto Spacing = 25_m;
		for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
		for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
			auto offset = make_translate({x, y, 0 * 0.001});
			engine.enqueue("block"_id, std::array{
				gfx::Engine::Instance{
					.transform = offset * transform1 * prescale,
					.tint = {0.9f, 0.9f, 1.0f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform2 * prescale,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform3 * prescale,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform4 * prescale,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform5 * prescale,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform55 * prescale,
					.tint = {0.1f, 0.1f, 0.9f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform6 * prescale,
					.tint = {0.2f, 0.9f, 0.5f, 1.0f},
					.roughness = 0.2f,
					.metalness = 0.9f,
				},
			});
			for (auto i = 0.0f; i <= 1.0f; i += 0.125f) {
				auto offset2 = offset * make_translate({(i - 0.5f) * 2.0f * 8_m, 0_m, 0_m});
				engine.enqueue("sphere"_id, std::array{gfx::Engine::Instance{
					.transform = offset2 * transform7 * prescale,
					.tint = {1.0f, 1.0f, 1.0f, 1.0f},
					.roughness = i,
					.metalness = 0.9f,
				}});
				engine.enqueue("sphere"_id, std::array{gfx::Engine::Instance{
					.transform = offset2 * transform8 * prescale,
					.tint = {1.0f, 1.0f, 1.0f, 1.0f},
					.roughness = i,
					.metalness = 0.1f,
				}});
			}
		}

		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
