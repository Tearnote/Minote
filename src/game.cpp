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
		static auto spin = false;
		static auto cameraDistance = 32.0f;
		static auto cameraPitch = radians(20.0f);
		static auto cameraYaw = radians(13.333f);
#if IMGUI
		ImGui::Checkbox("Spin the camera", &spin);
		ImGui::SliderFloat("Camera distance", &cameraDistance, 20.0f, 10000.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderAngle("Camera pitch", &cameraPitch, 0.0f, 89.875f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderAngle("Camera yaw", &cameraYaw, -180.0f, 180.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
#endif //IMGUI
		if (spin) {
			engine.setCamera({std::sin(glfwGetTime() / 4.0) * 28.0f, std::cos(glfwGetTime() / 4.0) * 28.0f, std::sin(glfwGetTime() / 3.1) * 4.0f + 12.0f}, {0.0f, 0.0f, 4.0f});
		} else {
			auto cameraPosition = vec3(0.0f, -cameraDistance, 0.0f);
			cameraPosition = glm::mat3(make_rotate(cameraPitch, {-1.0f, 0.0f, 0.0f})) * cameraPosition;
			cameraPosition = glm::mat3(make_rotate(cameraYaw, {0.0f, 0.0f, 1.0f})) * cameraPosition;
			engine.setCamera(cameraPosition, {0.0f, 0.0f, 4.0f});
		}

		ImGui::SliderInt("Expand", &Expand, 0, 40);

		auto rotateTransform = make_rotate(radians(180.0f), {1.0f, 0.0f, 0.0f});
		auto rotateTransformAnim = make_rotate(f32(radians(f64(sys::Glfw::getTime().count()) / 20000000.0)), {0.0f, 0.0f, 1.0f});
		auto transform1 = make_translate({0.0f, 0.0f, 0.0f}) * make_scale({12.0f, 12.0f, 1.0f}) * inverse(rotateTransform);
		auto transform2 = make_translate({-4.0f, -4.0f, 2.0f}) * rotateTransform;
		auto transform3 = make_translate({4.0f, -4.0f, 2.0f}) * rotateTransform;
		auto transform4 = make_translate({-4.0f, 4.0f, 2.0f}) * rotateTransform;
		auto transform5 = make_translate({4.0f, 4.0f, 2.0f}) * rotateTransform;
		auto transform55 = make_translate({7.0f, 0.0f, 2.0f}) * rotateTransform;
		auto transform6 = make_translate({0.0f, 0.0f, 2.5f}) * make_scale({1.5f, 1.5f, 1.5f}) * rotateTransformAnim * rotateTransform;
		auto transform7 = make_translate({0.0f, 8.0f, 2.0f});
		auto transform8 = make_translate({0.0f, -8.0f, 2.0f});
		constexpr auto Spacing = 25.0f;
		for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
		for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
			auto offset = make_translate({x, y, 0.0f});
			engine.enqueue("block"_id, std::array{
				gfx::Engine::Instance{
					.transform = offset * transform1,
					.tint = {0.9f, 0.9f, 1.0f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform2,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform3,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform4,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform5,
					.tint = {0.9f, 0.1f, 0.1f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform55,
					.tint = {0.1f, 0.1f, 0.9f, 1.0f},
					.roughness = 0.6f,
					.metalness = 0.1f,
				},
				gfx::Engine::Instance{
					.transform = offset * transform6,
					.tint = {0.2f, 0.9f, 0.5f, 1.0f},
					.roughness = 0.2f,
					.metalness = 0.9f,
				},
			});
			for (auto i = 0.0f; i <= 1.0f; i += 0.125f) {
				auto offset2 = offset * make_translate({(i - 0.5f) * 2.0f * 8.0f, 0.0f, 0.0f});
				engine.enqueue("sphere"_id, std::array{gfx::Engine::Instance{
					.transform = offset2 * transform7,
					.tint = {1.0f, 1.0f, 1.0f, 1.0f},
					.roughness = i,
					.metalness = 0.9f,
				}});
				engine.enqueue("sphere"_id, std::array{gfx::Engine::Instance{
					.transform = offset2 * transform8,
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
