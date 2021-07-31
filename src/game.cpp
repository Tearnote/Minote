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
#include "sys/vulkan.hpp"
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
	
	auto meshList = gfx::MeshList();
	auto assets = Assets(AssetsPath);
	assets.loadModels([&meshList](auto name, auto data) {
		
		meshList.addGltf(name, data);
		
	});
	
	auto vulkan = sys::Vulkan(window);
	auto engine = gfx::Engine(vulkan, std::move(meshList));
	
	engine.camera() = gfx::Camera{
		.position = {-10_m, -26_m, 64_m + 10_m},
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
	
	auto dynamicObjects = std::vector<gfx::ObjectID>();
	auto dynamicObjectPositions = std::vector<vec3>();
	defer {
		for (auto id: dynamicObjects)
			engine.objects().destroy(id);
	};
	
	auto const Expand = 20u;
	auto prescale = vec3{1_m, 1_m, 1_m};
	auto rotation = mat3::rotate({1.0f, 0.0f, 0.0f}, 180_deg);
	constexpr auto Spacing = 25_m;
	for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
	for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
		auto offset = vec3{x, y, 64_m};
		
		auto block1 = engine.objects().create();
		engine.objects().meshID[block1] = "block"_id;
		engine.objects().transform[block1] = 
			mat4::translate(vec3{0_m, 0_m, 0_m} + offset) *
			mat4(rotation) *
			mat4::scale(vec3{12.0f, 12.0f, 1.0f} * prescale);
		engine.objects().material[block1] = gfx::Objects::Material{
			.tint = {0.9f, 0.9f, 1.0f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f};
		
		auto block2 = engine.objects().create();
		engine.objects().meshID[block2] = "block"_id;
		engine.objects().transform[block2] = 
			mat4::translate(vec3{-4_m, -4_m, 2_m} + offset) *
			mat4(rotation) *
			mat4::scale(prescale);
		engine.objects().material[block2] = gfx::Objects::Material{
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f};
		
		auto block3 = engine.objects().create();
		engine.objects().meshID[block3] = "block"_id;
		engine.objects().transform[block3] = 
			mat4::translate(vec3{4_m, -4_m, 2_m} + offset) *
			mat4(rotation) *
			mat4::scale(prescale);
		engine.objects().material[block3] = gfx::Objects::Material{
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f};
		
		auto block4 = engine.objects().create();
		engine.objects().meshID[block4] = "block"_id;
		engine.objects().transform[block4] = 
			mat4::translate(vec3{-4_m, 4_m, 2_m} + offset) *
			mat4(rotation) *
			mat4::scale(prescale);
		engine.objects().material[block4] = gfx::Objects::Material{
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f};
		
		auto block5 = engine.objects().create();
		engine.objects().meshID[block5] = "block"_id;
		engine.objects().transform[block5] = 
			mat4::translate(vec3{4_m, 4_m, 2_m} + offset) *
			mat4(rotation) *
			mat4::scale(prescale);
		engine.objects().material[block5] = gfx::Objects::Material{
			.tint = {0.9f, 0.1f, 0.1f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f};
		
		auto block6 = engine.objects().create();
		engine.objects().meshID[block6] = "block"_id;
		engine.objects().transform[block6] = 
			mat4::translate(vec3{7_m, 0_m, 2_m} + offset) *
			mat4(rotation) *
			mat4::scale(prescale);
		engine.objects().material[block6] = gfx::Objects::Material{
			.tint = {0.1f, 0.1f, 0.9f, 1.0f},
			.roughness = 0.6f,
			.metalness = 0.1f};
		
		auto block7 = engine.objects().create();
		dynamicObjects.emplace_back(block7);
		dynamicObjectPositions.emplace_back(vec3{0_m, 0_m, 2.5_m} + offset);
		engine.objects().meshID[block7] = "block"_id;
		engine.objects().material[block7] = gfx::Objects::Material{
			.tint = {0.2f, 0.9f, 0.5f, 1.0f},
			.roughness = 0.2f,
			.metalness = 0.9f};
		
		for (auto i = 0.0f; i <= 1.0f; i += 0.125f) {
			auto offset2 = offset + vec3{(i - 0.5f) * 2.0f * 8_m, 0_m, 0_m};
			
			auto sphere1 = engine.objects().create();
			engine.objects().meshID[sphere1] = "sphere"_id;
			engine.objects().transform[sphere1] = 
				mat4::translate(vec3{0_m, 8_m, 2_m} + offset2) *
				mat4::scale(prescale);
			engine.objects().material[sphere1] = gfx::Objects::Material{
				.tint = {1.0f, 1.0f, 1.0f, 1.0f},
				.roughness = i,
				.metalness = 0.9f};
			
			auto sphere2 = engine.objects().create();
			engine.objects().meshID[sphere2] = "sphere"_id;
			engine.objects().transform[sphere2] = 
				mat4::translate(vec3{0_m, -8_m, 2_m} + offset2) *
				mat4::scale(prescale);
			engine.objects().material[sphere2] = gfx::Objects::Material{
				.tint = {1.0f, 1.0f, 1.0f, 1.0f},
				.roughness = i,
				.metalness = 0.1f};
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
				
				if (action.state == Mapper::Action::State::Pressed && ImGui::GetIO().WantCaptureKeyboard)
					return false;
				
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
			if (!window.mouseDown() || ImGui::GetIO().WantCaptureMouse)
				lastButtonState = false;
			if (lastButtonState) {
				cursorOffset += cursorNewPos - cursorLastPos;
				cursorLastPos = cursorNewPos;
			}
			cursorOffset.y() = -cursorOffset.y();
			
			engine.camera().rotate(cursorOffset.x(), cursorOffset.y());
			engine.camera().roam({
				float(camRight) - float(camLeft),
				0.0f,
				float(camUp) - float(camDown)});
			engine.camera().shift({0.0f, 0.0f, float(camFloat)});
		}
		
		// Graphics
		
		{
			OPTICK_EVENT("Update spinny squares");
			auto rotateTransform = mat3::rotate({1.0f, 0.0f, 0.0f}, 180_deg);
			auto rotateTransformAnim = mat3::rotate({0.0f, 0.0f, 1.0f}, radians(ratio(sys::Glfw::getTime(), 20_ms)));
			for (auto& obj: dynamicObjects) {
			engine.objects().transform[obj] = 
				mat4::translate(dynamicObjectPositions[&obj - &dynamicObjects[0]]) *
				mat4(rotateTransformAnim * rotateTransform) *
				mat4::scale(vec3{1.5f, 1.5f, 1.5f} * prescale);
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
