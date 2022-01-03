#include "game.hpp"

#include "config.hpp"

#include <exception>
#include "Tracy.hpp"
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"
#include "base/containers/vector.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "gfx/models.hpp"
#include "assets.hpp"
#include "main.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

// Rate of the logic update clock. Can be higher than display refresh rate.
static constexpr auto LogicRate = 120;
static constexpr auto LogicTick = 1_s / LogicRate;

void game(GameParams const& _params) try {
	
	ZoneScoped;
	tracy::SetThreadName("Game");
	
	auto& window = _params.window;
	auto& engine = _params.engine;
	auto& mapper = _params.mapper;
	
	// Load assets
	
	auto modelList = gfx::ModelList();
	auto assets = Assets(Assets_p);
	assets.loadModels([&modelList](auto name, auto data) {
		
		modelList.addModel(name, data);
		
	});
	
	// Initialize the engine
	
	engine.init(std::move(modelList));
	
	engine.camera() = gfx::Camera{
		.position = {8.57_m, -16.07_m, 69.20_m},
		.yaw = 2.41412449f,
		.pitch = 0.113862038f,
		.lookSpeed = 1.0f / 256.0f,
		.moveSpeed = 1_m / 16.0f};
	
	// Makeshift freeform camera controls
	auto camUp = false;
	auto camDown = false;
	auto camLeft = false;
	auto camRight = false;
	auto camFloat = false;
	auto camMoving = false;
	auto camOffset = vec2(0.0f);
	
	// Create test scene
	
	// Keep track of the spinning cubes so that we can rotate them each frame
	auto dynamicObjects = ivector<gfx::ObjectID>();
	defer {
		for (auto id: dynamicObjects)
			engine.objects().destroy(id);
	};
	
	constexpr auto prescale = vec3{1_m, 1_m, 1_m};
	constexpr auto Expand = 0u;
	constexpr auto Spacing = 25_m;
	
	constexpr auto TestScenes = 2;
	for (auto x: iota(0, TestScenes))
	for (auto y: iota(0, TestScenes)) {
		
		constexpr auto TestSpacing = 80_m;
		auto offset = vec3{x * 80_m, y * 80_m, 0.0f};
		
		auto testscene_id = engine.objects().create();
		auto testscene = engine.objects().get(testscene_id);
		testscene.modelID = "testscene"_id;
		testscene.transform.position = vec3{0_m, 0_m, 64_m} + offset;
		testscene.transform.scale = prescale;
		
	}
	
	// Create another test scene
	
	for (auto x = -Spacing * Expand; x <= Spacing * Expand; x += Spacing)
	for (auto y = -Spacing * Expand; y <= Spacing * Expand; y += Spacing) {
		
		auto offset = vec3{x, y, 32_m};
		
		auto block1_id = engine.objects().create();
		auto block1 = engine.objects().get(block1_id);
		block1.modelID = "block"_id;
		block1.color = {0.9f, 0.9f, 1.0f, 1.0f};
		block1.transform.position = offset;
		block1.transform.scale = vec3{12.0f, 12.0f, 1.0f} * prescale;
		
		auto block2_id = engine.objects().create();
		auto block2 = engine.objects().get(block2_id);
		block2.modelID = "block"_id;
		block2.color = {0.9f, 0.1f, 0.1f, 1.0f};
		block2.transform.position = vec3{-4_m, -4_m, 2_m} + offset;
		block2.transform.scale = prescale;
		
		auto block3_id = engine.objects().create();
		auto block3 = engine.objects().get(block3_id);
		block3.modelID = "block"_id;
		block3.color = {0.9f, 0.1f, 0.1f, 1.0f};
		block3.transform.position = vec3{4_m, -4_m, 2_m} + offset;
		block3.transform.scale = prescale;
		
		auto block4_id = engine.objects().create();
		auto block4 = engine.objects().get(block4_id);
		block4.modelID = "block"_id;
		block4.color = {0.9f, 0.1f, 0.1f, 1.0f};
		block4.transform.position = vec3{-4_m, 4_m, 2_m} + offset;
		block4.transform.scale = prescale;
		
		auto block5_id = engine.objects().create();
		auto block5 = engine.objects().get(block5_id);
		block5.modelID = "block"_id;
		block5.color = {0.9f, 0.1f, 0.1f, 1.0f};
		block5.transform.position = vec3{4_m, 4_m, 2_m} + offset;
		block5.transform.scale = prescale;
		
		auto block6_id = engine.objects().create();
		auto block6 = engine.objects().get(block6_id);
		block6.modelID = "block"_id;
		block6.color = {0.1f, 0.1f, 0.9f, 1.0f};
		block6.transform.position = vec3{7_m, 0_m, 2_m} + offset;
		block6.transform.scale = prescale;
		
		auto block7_id = engine.objects().create();
		auto block7 = engine.objects().get(block7_id);
		dynamicObjects.emplace_back(block7_id);
		block7.modelID = "block"_id;
		block7.color = {0.2f, 0.9f, 0.5f, 1.0f};
		block7.transform.position = vec3{0_m, 0_m, 2.5_m} + offset;
		block7.transform.scale = vec3{1.5f, 1.5f, 1.5f} * prescale;
		
		for (auto i: iota(0, 9)) {
			
			auto offset2 = offset + vec3{f32(i - 4) / 4.0f * 8_m, 0_m, 0_m};
			
			auto sphere1_id = engine.objects().create();
			auto sphere1 = engine.objects().get(sphere1_id);
			sphere1.modelID = "sphere"_id;
			sphere1.transform.position = vec3{0_m, 8_m, 2_m} + offset2;
			sphere1.transform.scale = prescale;
			
			auto sphere2_id = engine.objects().create();
			auto sphere2 = engine.objects().get(sphere2_id);
			sphere2.modelID = "sphere"_id;
			sphere2.transform.position = vec3{0_m, -8_m, 2_m} + offset2;
			sphere2.transform.scale = prescale;
			
		}
		
	}
	
	L_INFO("Game initialized");
	
	// Main loop
	
	auto nextUpdate = sys::System::getTime();
	
	while (!sys::System::isQuitting()) {
		
		auto framerateScale = min(144.0f / engine.fps(), 8.0f);
		engine.camera().moveSpeed = 1_m / 16.0f * framerateScale;
		
		// Input handling
		
		ImGui_ImplSDL2_NewFrame(window.handle());
		
		while (nextUpdate <= sys::System::getTime()) {
			
			FrameMarkStart("Logic");
			
			// Iterate over all events
			
			sys::System::forEachEvent([&] (const sys::System::Event& e) -> bool {
				
				// Don't handle events from the future
				if (milliseconds(e.common.timestamp) > nextUpdate) return false;
				
				// Leave quit events alone
				if (e.type == SDL_QUIT) return false;
				
				// Let ImGui handle all events it uses
				ImGui_ImplSDL2_ProcessEvent(&e);
				
				// If ImGui wants exclusive control, leave now
				if (e.type == SDL_KEYDOWN)
					if (ImGui::GetIO().WantCaptureKeyboard) return true;
				if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION)
					if (ImGui::GetIO().WantCaptureMouse) return true;
				
				// Camera motion
				if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
					camMoving = true;
				if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
					camMoving = false;
				if (e.type == SDL_MOUSEMOTION)
					camOffset += vec2{f32(e.motion.xrel), f32(e.motion.yrel)};
				
				// Game logic events
				if (auto action = mapper.convert(e)) {
					
					// Quit event
					if (action->type == Action::Type::Back)
						sys::System::postQuitEvent();
					
					// Placeholder camera input
					auto state = (action->state == Mapper::Action::State::Pressed);
					if (action->type == Action::Type::Drop)
						camUp = state;
					if (action->type == Action::Type::Lock)
						camDown = state;
					if (action->type == Action::Type::Left)
						camLeft = state;
					if (action->type == Action::Type::Right)
						camRight = state;
					if (action->type == Action::Type::Skip)
						camFloat = state;
					
				}
				
				return true;
				
			});
			
			nextUpdate += LogicTick;
			
			FrameMarkEnd("Logic");
			
		}
		
		// Logic
		
		// Camera update
		camOffset.y() = -camOffset.y();
		
		if (camMoving)
			engine.camera().rotate(camOffset.x(), camOffset.y());
		
		engine.camera().roam({
			float(camRight) - float(camLeft),
			0.0f,
			float(camUp) - float(camDown)});
		engine.camera().shift({0.0f, 0.0f, float(camFloat)});
		
		camOffset = vec2(0.0f);
		
		// Graphics
		
		{
			
			ZoneScopedN("Rotating blocks");
			auto rotateAnim = quat::angleAxis(radians(ratio(sys::System::getTime(), 20_ms)), {0.0f, 0.0f, 1.0f});
			for (auto& obj: dynamicObjects)
				engine.objects().get(obj).transform.rotation = rotateAnim;
			
		}
		
		engine.render();
		
	}
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on game thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	sys::System::postQuitEvent();
	
}

}
