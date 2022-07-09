#include "game.hpp"

#include "config.hpp"

#include <exception>
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"
#include "util/vector.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "util/log.hpp"
#include "gfx/models.hpp"
#include "assets.hpp"
#include "scenes.hpp"
#include "main.hpp"

namespace minote {

// Rate of the logic update clock. Can be higher than display refresh rate.
static constexpr auto LogicRate = 120;
static constexpr auto LogicTick = 1_s / LogicRate;

static auto loadAssets(string_view _path) -> ModelList {
	
	auto modelList = ModelList();
	auto assets = Assets(_path);
	assets.loadModels([&modelList](auto name, auto data) {
		modelList.addModel(name, data);
	});
	
	return modelList;
	
}

void game(GameParams const& _params) try {
	
	auto& window = _params.window;
	auto& engine = *s_engine;
	auto& mapper = _params.mapper;
	
	auto modelList = loadAssets(Assets_p);
	engine.init(std::move(modelList));
	
	engine.camera() = Camera{
		.position = {8.57_m, -16.07_m, 69.20_m},
		.yaw = 2.41412449f,
		.pitch = 0.113862038f,
		.lookSpeed = 1.0f / 256.0f,
		.moveSpeed = 1_m / 16.0f,
	};
	
	// Makeshift freeform camera controls
	auto camUp = false;
	auto camDown = false;
	auto camLeft = false;
	auto camRight = false;
	auto camFloat = false;
	auto camMoving = false;
	auto camOffset = vec2(0.0f);
	
	// Create test scene
	
	constexpr auto Prescale = vec3{1_m, 1_m, 1_m};
	
	constexpr auto BattleScenes = 1;
	constexpr auto BattleSpacing = 80_m;
	auto battleScenes = svector<BattleScene, BattleScenes*BattleScenes>();
	for (auto x: iota(0, BattleScenes))
	for (auto y: iota(0, BattleScenes)) {
		battleScenes.emplace_back(Transform{
			.position = vec3{x * BattleSpacing, y * BattleSpacing, 64_m},
			.scale = Prescale,
		});
	}
	
	// Create another test scene
	
	constexpr auto TestExpand = 0u;
	constexpr auto TestSpacing = 25_m;
	auto testScenes = svector<SimpleScene, (TestExpand * 2 + 1) * (TestExpand * 2 + 1)>();
	for (auto x = -TestSpacing * TestExpand; x <= TestSpacing * TestExpand; x += TestSpacing)
	for (auto y = -TestSpacing * TestExpand; y <= TestSpacing * TestExpand; y += TestSpacing) {
		testScenes.emplace_back(Transform{
			.position = vec3{x, y, 32_m},
			.scale = Prescale,
		});
	}
	
	L_INFO("Game initialized");
	
	// Main loop
	
	auto nextUpdate = System::getTime();
	
	while (!System::isQuitting()) {
		
		auto framerateScale = min(144.0f / engine.fps(), 8.0f);
		engine.camera().moveSpeed = 1_m / 16.0f * framerateScale;
		
		// Input handling
		
		ImGui_ImplSDL2_NewFrame(window.handle());
		
		while (nextUpdate <= System::getTime()) {
			
			// Iterate over all events
			
			System::forEachEvent([&] (const System::Event& e) -> bool {
				
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
					if (action->type == Mapper::Action::Type::Back)
						System::postQuitEvent();
					
					// Placeholder camera input
					auto state = (action->state == Mapper::Action::State::Pressed);
					if (action->type == Mapper::Action::Type::Drop)
						camUp = state;
					if (action->type == Mapper::Action::Type::Lock)
						camDown = state;
					if (action->type == Mapper::Action::Type::Left)
						camLeft = state;
					if (action->type == Mapper::Action::Type::Right)
						camRight = state;
					if (action->type == Mapper::Action::Type::Skip)
						camFloat = state;
					
				}
				
				return true;
				
			});
			
			nextUpdate += LogicTick;
			
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
		
		engine.render();
		
	}
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on game thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	System::postQuitEvent();
	
}

}
