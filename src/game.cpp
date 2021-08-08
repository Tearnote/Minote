#include "game.hpp"

#include "config.hpp"

#include <exception>
#include "GLFW/glfw3.h"
#include "optick.h"
#include "imgui.h"
#include "base/container/vector.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "gfx/engine.hpp"
#include "sys/vulkan.hpp"
#include "assets.hpp"
#include "mapper.hpp"
#include "memory.hpp"
#include "main.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

// Rate of the logic update clock. Can be higher than display refresh rate.
static constexpr auto LogicRate = 120;
static constexpr auto LogicTick = 1_s / LogicRate;

void game(sys::Window& _window) try {
	
	OPTICK_THREAD("Game");
	
	// Initialize core systems
	
	attachPoolResources();
	
	auto mapper = Mapper();
	
	auto vulkan = sys::Vulkan(_window);
	
	// Load assets
	
	auto meshList = gfx::MeshList();
	auto assets = Assets(Assets_p);
	assets.loadModels([&meshList](auto name, auto data) {
		
		meshList.addGltf(name, data);
		
	});
	
	// Start up graphics engine
	
	auto engine = gfx::Engine(vulkan, std::move(meshList));
	
	engine.camera() = gfx::Camera{
		.position = {-10_m, -26_m, 64_m + 10_m},
		.yaw = 58_deg,
		.pitch = -12_deg,
		.lookSpeed = 1.0f / 256.0f,
		.moveSpeed = 1_m / 16.0f};
	
	// Makeshift freeform camera controls
	auto camUp = false;
	auto camDown = false;
	auto camLeft = false;
	auto camRight = false;
	auto camFloat = false;
	auto cursorLastPos = vec2();
	auto lastButtonState = false;
	
	// Create test scene
	
	// Keep track of the spinning cubes so that we can rotate them each frame
	auto dynamicObjects = ivector<gfx::ObjectID>();
	defer {
		for (auto id: dynamicObjects)
			engine.objects().destroy(id);
	};
	
	constexpr auto prescale = vec3{1_m, 1_m, 1_m};
	constexpr auto rotation = quat::angleAxis(180_deg, {1.0f, 0.0f, 0.0f});
	constexpr auto Expand = 20u;
	constexpr auto Spacing = 25_m;
	for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
	for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
		
		auto offset = vec3{x, y, 64_m};
		
		auto block1_id = engine.objects().create();
		auto block1 = engine.objects().get(block1_id);
		block1.meshID = "block"_id;
		block1.transform.position = offset;
		block1.transform.scale = vec3{12.0f, 12.0f, 1.0f} * prescale;
		block1.transform.rotation = rotation;
		block1.material.tint = {0.9f, 0.9f, 1.0f, 1.0f};
		block1.material.roughness = 0.6f;
		block1.material.metalness = 0.1f;
		
		auto block2_id = engine.objects().create();
		auto block2 = engine.objects().get(block2_id);
		block2.meshID = "block"_id;
		block2.transform.position = vec3{-4_m, -4_m, 2_m} + offset;
		block2.transform.scale = prescale;
		block2.transform.rotation = rotation;
		block2.material.tint = {0.9f, 0.1f, 0.1f, 1.0f};
		block2.material.roughness = 0.6f;
		block2.material.metalness = 0.1f;
		
		auto block3_id = engine.objects().create();
		auto block3 = engine.objects().get(block3_id);
		block3.meshID = "block"_id;
		block3.transform.position = vec3{4_m, -4_m, 2_m} + offset;
		block3.transform.scale = prescale;
		block3.transform.rotation = rotation;
		block3.material.tint = {0.9f, 0.1f, 0.1f, 1.0f};
		block3.material.roughness = 0.6f;
		block3.material.metalness = 0.1f;
		
		auto block4_id = engine.objects().create();
		auto block4 = engine.objects().get(block4_id);
		block4.meshID = "block"_id;
		block4.transform.position = vec3{-4_m, 4_m, 2_m} + offset;
		block4.transform.scale = prescale;
		block4.transform.rotation = rotation;
		block4.material.tint = {0.9f, 0.1f, 0.1f, 1.0f};
		block4.material.roughness = 0.6f;
		block4.material.metalness = 0.1f;
		
		auto block5_id = engine.objects().create();
		auto block5 = engine.objects().get(block5_id);
		block5.meshID = "block"_id;
		block5.transform.position = vec3{4_m, 4_m, 2_m} + offset;
		block5.transform.scale = prescale;
		block5.transform.rotation = rotation;
		block5.material.tint = {0.9f, 0.1f, 0.1f, 1.0f};
		block5.material.roughness = 0.6f;
		block5.material.metalness = 0.1f;
		
		auto block6_id = engine.objects().create();
		auto block6 = engine.objects().get(block6_id);
		block6.meshID = "block"_id;
		block6.transform.position = vec3{7_m, 0_m, 2_m} + offset;
		block6.transform.scale = prescale;
		block6.transform.rotation = rotation;
		block6.material.tint = {0.1f, 0.1f, 0.9f, 1.0f};
		block6.material.roughness = 0.6f;
		block6.material.metalness = 0.1f;
		
		auto block7_id = engine.objects().create();
		auto block7 = engine.objects().get(block7_id);
		dynamicObjects.emplace_back(block7_id);
		block7.meshID = "block"_id;
		block7.transform.position = vec3{0_m, 0_m, 2.5_m} + offset;
		block7.transform.scale = vec3{1.5f, 1.5f, 1.5f} * prescale;
		block7.material.tint = {0.2f, 0.9f, 0.5f, 1.0f};
		block7.material.roughness = 0.2f;
		block7.material.metalness = 0.9f;
		
		for (auto i = 0.0f; i <= 1.0f; i += 0.125f) {
			
			auto offset2 = offset + vec3{(i - 0.5f) * 2.0f * 8_m, 0_m, 0_m};
			
			auto sphere1_id = engine.objects().create();
			auto sphere1 = engine.objects().get(sphere1_id);
			sphere1.meshID = "sphere"_id;
			sphere1.transform.position = vec3{0_m, 8_m, 2_m} + offset2;
			sphere1.transform.scale = prescale;
			sphere1.material.roughness = i;
			sphere1.material.metalness = 0.9f;
			
			auto sphere2_id = engine.objects().create();
			auto sphere2 = engine.objects().get(sphere2_id);
			sphere2.meshID = "sphere"_id;
			sphere2.transform.position = vec3{0_m, -8_m, 2_m} + offset2;
			sphere2.transform.scale = prescale;
			sphere2.material.roughness = i;
			sphere2.material.metalness = 0.1f;
			
		}
		
	}
	
	L_INFO("Game initialized");
	
	// Main loop
	
	auto nextUpdate = sys::Glfw::getTime();
	
	while (!_window.isClosing()) {
		
		OPTICK_FRAME("Renderer");
		
		// Input
		mapper.collectKeyInputs(_window);
		
		// Logic
		auto updateActions = ivector<Mapper::Action, 256, PerFrame>();
		while (nextUpdate <= sys::Glfw::getTime()) {
			
			// Pull all suitable input events from the queue
			mapper.processActions([&](auto const& action) {
				
				if (action.timestamp > nextUpdate) return false;
				
				if (action.state == Mapper::Action::State::Pressed && ImGui::GetIO().WantCaptureKeyboard)
					return false;
				
				updateActions.push_back(action);
				
				// Interpret quit events here for now
				using Action = Mapper::Action::Type;
				if (action.type == Action::Back)
					_window.requestClose();
				
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
			
			nextUpdate += LogicTick;
			
			// Placeholder camera controls
			auto cursorNewPos = _window.mousePos();
			auto cursorOffset = vec2();
			if (!lastButtonState && _window.mouseDown()) {
				
				lastButtonState = true;
				cursorLastPos = cursorNewPos;
				
			}
			
			if (!_window.mouseDown() || ImGui::GetIO().WantCaptureMouse)
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
			constexpr auto rotateTransform = quat::angleAxis(180_deg, {1.0f, 0.0f, 0.0f});
			auto rotateTransformAnim = quat::angleAxis(radians(ratio(sys::Glfw::getTime(), 20_ms)), {0.0f, 0.0f, 1.0f});
			for (auto& obj: dynamicObjects)
				engine.objects().get(obj).transform.rotation = rotateTransformAnim * rotateTransform;
			
		}
		
		engine.render();
		
		// Cleanup frame resources
		
		resetPerFrameAllocator();
		
	}
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on game thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	_window.requestClose();
	
}

}
