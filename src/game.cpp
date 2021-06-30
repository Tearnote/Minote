#include "game.hpp"

#include "config.hpp"

#include <exception>
#include <vector>
#include "GLFW/glfw3.h"
#include "optick.h"
#include "imgui.h"
#include "base/math.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "gfx/engine.hpp"
#include "assets.hpp"
#include "mapper.hpp"
//#include "playstate.hpp"
#include "memory.hpp"
#include "main.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

constexpr auto UpdateTick = 1_s / 120;

void game(sys::Glfw&, sys::Window& window) try {
	
	OPTICK_THREAD("Game");

	// *** Initialization ***
	
	attachPoolResources();

	auto mapper = Mapper();
	auto engine = gfx::Engine(window, AppVersion);

	auto assets = Assets(AssetsPath);
	assets.loadModels([&engine](auto name, auto data) {
		engine.meshes->addGltf(name, data);
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
	
	auto staticObjects = std::vector<gfx::ObjectID>();
	defer {
		for (auto id: staticObjects)
			engine.objects->destroy(id);
	};
	auto dynamicObjects = std::vector<gfx::Object>();
	defer {
		for (auto& obj: dynamicObjects)
			engine.objects->destroy(obj);
	};
	
	auto const Expand = 20u;
	auto prescale = vec3{1_m, 1_m, 1_m};
	auto rotation = mat3::rotate({1.0f, 0.0f, 0.0f}, 180_deg);
	constexpr auto Spacing = 25_m;
	for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
	for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
		auto offset = vec3{x, y, 0};
		staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{0_m, 0_m, 0_m} + offset,
			.scale = vec3{12.0f, 12.0f, 1.0f} * prescale,
			.rotation = rotation,
			.tint = {0.9f, 0.9f, 1.0f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f}));
		staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{-4_m, -4_m, 2_m} + offset,
			.scale = prescale,
			.rotation = rotation,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f}));
		staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{4_m, -4_m, 2_m} + offset,
			.scale = prescale,
			.rotation = rotation,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f}));
		staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{-4_m, 4_m, 2_m} + offset,
			.scale = prescale,
			.rotation = rotation,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f}));
		staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{4_m, 4_m, 2_m} + offset,
			.scale = prescale,
			.rotation = rotation,
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f}));
		staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{7_m, 0_m, 2_m} + offset,
			.scale = prescale,
			.rotation = rotation,
			.tint = {0.1f, 0.1f, 0.9f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f}));
		dynamicObjects.emplace_back(engine.objects->createDynamic(gfx::Object{
			.mesh = "block"_id,
			.position = vec3{0_m, 0_m, 2.5_m} + offset,
			.scale = vec3{1_m, 1_m, 1_m} * vec3{1.5f, 1.5f, 1.5f},
			.tint = {0.2f, 0.9f, 0.5f, 1.0f},
			.roughness = 0.2f,
			.metalness = 0.9f}));
		for (auto i = 0.0f; i <= 1.0f; i += 0.125f) {
			auto offset2 = offset + vec3{(i - 0.5f) * 2.0f * 8_m, 0_m, 0_m};
			staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
				.mesh = "sphere"_id,
				.position = vec3{0_m, 8_m, 2_m} + offset2,
				.scale = prescale,
				.tint = {1.0f, 1.0f, 1.0f, 1.0f},
				.roughness = i,
				.metalness = 0.9f}));
			staticObjects.emplace_back(engine.objects->createStatic(gfx::Object{
				.mesh = "sphere"_id,
				.position = vec3{0_m, -8_m, 2_m} + offset2,
				.scale = prescale,
				.tint = {1.0f, 1.0f, 1.0f, 1.0f},
				.roughness = i,
				.metalness = 0.1f}));
		}
	}
	
	// *** Main loop ***
	
	auto nextUpdate = sys::Glfw::getTime();
	
	while (!window.isClosing()) {
		OPTICK_FRAME("Renderer");
		
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
			cursorOffset.y() = -cursorOffset.y();
			
			engine.camera.rotate(cursorOffset.x(), cursorOffset.y());
			engine.camera.roam({
				float(camRight) - float(camLeft),
				0.0f,
				float(camUp) - float(camDown)});
			engine.camera.shift({0.0f, 0.0f, float(camFloat)});
		}
		
		// Graphics
		
		{
			OPTICK_EVENT("Update spinny squares");
			auto rotateTransform = mat3::rotate({1.0f, 0.0f, 0.0f}, 180_deg);
			auto rotateTransformAnim = mat3::rotate({0.0f, 0.0f, 1.0f}, f32(radians(f64(sys::Glfw::getTime().count()) / 20000000.0)));
			for (auto& obj: dynamicObjects) {
				obj.rotation = rotateTransformAnim * rotateTransform;
				engine.objects->update(obj);
			}
		}
		
		engine.render();
	}
	
} catch (std::exception const& e) {
	L_CRIT("Unhandled exception on game thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	window.requestClose();
}

}
