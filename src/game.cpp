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
		engine.meshes.addGltf(name, data);
	});
	engine.uploadAssets();
	
	engine.camera = gfx::Camera{
		.position = {-10_m, -26_m, 10_m},
		.yaw = 58_deg,
		.pitch = -12_deg,
		.lookSpeed = 1.0f / 256.0f,
		.moveSpeed = 1_m / 16.0f,
	};
	auto camUp = false;
	auto camDown = false;
	auto camLeft = false;
	auto camRight = false;
	auto camFloat = false;
	auto cursorLastPos = vec2();
	auto lastButtonState = false;
	
	// PlayState play;
	
	// Create renderable objects
	
	auto const Expand = 0u;
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
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform1 * prescale,
			.tint = {0.9f, 0.9f, 1.0f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f});
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform2 * prescale,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f});
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform3 * prescale,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f});
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform4 * prescale,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f});
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform5 * prescale,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f});
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform55 * prescale,
			.tint = {0.1f, 0.1f, 0.9f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f});
		engine.objects.create({
			.mesh = "block"_id,
			.transform = offset * transform6 * prescale,
			.tint = {0.2f, 0.9f, 0.5f, 1.0f},
			.roughness = 0.2f,
			.metalness = 0.9f});
		for (auto i = 0.0f; i <= 1.0f; i += 0.125f) {
			auto offset2 = offset * make_translate({(i - 0.5f) * 2.0f * 8_m, 0_m, 0_m});
			engine.objects.create({
				.mesh = "sphere"_id,
				.transform = offset2 * transform7 * prescale,
				.tint = {1.0f, 1.0f, 1.0f, 1.0f},
				.roughness = i,
				.metalness = 0.9f});
			engine.objects.create({
				.mesh = "sphere"_id,
				.transform = offset2 * transform8 * prescale,
				.tint = {1.0f, 1.0f, 1.0f, 1.0f},
				.roughness = i,
				.metalness = 0.1f});
		}
	}
	
	// *** Main loop ***
	
	auto nextUpdate = sys::Glfw::getTime();
	
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
			auto cursorOffset = vec2();
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
			cursorOffset.y = -cursorOffset.y;
			
			engine.camera.rotate(cursorOffset.x, cursorOffset.y);
			engine.camera.roam({
				float(camRight) - float(camLeft),
				0.0f,
				float(camUp) - float(camDown)});
			engine.camera.shift({0.0f, 0.0f, float(camFloat)});
		}
		
		// Graphics
		
		engine.render();
	}

} catch (std::exception const& e) {
	L.crit("Unhandled exception on game thread: {}", e.what());
	L.crit("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
