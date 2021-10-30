#include "game.hpp"

#include "config.hpp"

#include <exception>
#include "Tracy.hpp"
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"
#include "base/containers/vector.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "base/ease.hpp"
#include "base/log.hpp"
#include "base/rng.hpp"
#include "gfx/materials.hpp"
#include "gfx/meshes.hpp"
#include "assets.hpp"
#include "main.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

// Rate of the logic update clock. Can be higher than display refresh rate.
static constexpr auto LogicRate = 120;
static constexpr auto LogicTick = 1_s / LogicRate;

vec3 HSVtoRGB(vec3 _in) {
  float fC = _in[2] * _in[1]; // Chroma
  float fHPrime = fmod(_in[0] / 60.0, 6);
  float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
  float fM = _in[2] - fC;
  
  vec3 result;
  
  if(0 <= fHPrime && fHPrime < 1) {
    result.r() = fC;
    result.g() = fX;
    result.b() = 0;
  } else if(1 <= fHPrime && fHPrime < 2) {
    result.r() = fX;
    result.g() = fC;
    result.b() = 0;
  } else if(2 <= fHPrime && fHPrime < 3) {
    result.r() = 0;
    result.g() = fC;
    result.b() = fX;
  } else if(3 <= fHPrime && fHPrime < 4) {
    result.r() = 0;
    result.g() = fX;
    result.b() = fC;
  } else if(4 <= fHPrime && fHPrime < 5) {
    result.r() = fX;
    result.g() = 0;
    result.b() = fC;
  } else if(5 <= fHPrime && fHPrime < 6) {
    result.r() = fC;
    result.g() = 0;
    result.b() = fX;
  } else {
    result.r() = 0;
    result.g() = 0;
    result.b() = 0;
  }
  
  result.r() += fM;
  result.g() += fM;
  result.b() += fM;
  
  return result;
}

void game(GameParams const& _params) try {
	
	ZoneScoped;
	tracy::SetThreadName("Game");
	
	auto& window = _params.window;
	auto& engine = _params.engine;
	auto& mapper = _params.mapper;
	
	// Load assets
	
	auto meshList = gfx::MeshList();
	auto assets = Assets(Assets_p);
	assets.loadModels([&meshList](auto name, auto data) {
		
		meshList.addGltf(name, data);
		
	});
	
	auto materialList = gfx::MaterialList();
	materialList.addMaterial("matte"_id, gfx::MaterialPBR::make({
		.roughness = 0.6f,
		.metalness = 0.0f }));
	materialList.addMaterial("shiny"_id, gfx::MaterialPBR::make({
		.roughness = 0.2f,
		.metalness = 1.0f }));
	materialList.addMaterial("plastic0"_id, gfx::MaterialPBR::make({
		.roughness = 0.0f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic1"_id, gfx::MaterialPBR::make({
		.roughness = 0.125f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic2"_id, gfx::MaterialPBR::make({
		.roughness = 0.25f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic3"_id, gfx::MaterialPBR::make({
		.roughness = 0.375f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic4"_id, gfx::MaterialPBR::make({
		.roughness = 0.5f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic5"_id, gfx::MaterialPBR::make({
		.roughness = 0.625f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic6"_id, gfx::MaterialPBR::make({
		.roughness = 0.75f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic7"_id, gfx::MaterialPBR::make({
		.roughness = 0.875f,
		.metalness = 0.0f }));
	materialList.addMaterial("plastic8"_id, gfx::MaterialPBR::make({
		.roughness = 0.875f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal0"_id, gfx::MaterialPBR::make({
		.roughness = 0.0f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal1"_id, gfx::MaterialPBR::make({
		.roughness = 0.125f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal2"_id, gfx::MaterialPBR::make({
		.roughness = 0.25f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal3"_id, gfx::MaterialPBR::make({
		.roughness = 0.375f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal4"_id, gfx::MaterialPBR::make({
		.roughness = 0.5f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal5"_id, gfx::MaterialPBR::make({
		.roughness = 0.625f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal6"_id, gfx::MaterialPBR::make({
		.roughness = 0.75f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal7"_id, gfx::MaterialPBR::make({
		.roughness = 0.875f,
		.metalness = 1.0f }));
	materialList.addMaterial("metal8"_id, gfx::MaterialPBR::make({
		.roughness = 0.875f,
		.metalness = 1.0f }));
	
	// Initialize the engine
	
	engine.init(std::move(meshList), std::move(materialList));
	
	engine.camera() = gfx::Camera{
		.position = {0.000520496164, -0.00399232749, 0.0640183836},
		.yaw = 1.64068699,
		.pitch = -0.0384817719,
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
	
	constexpr auto prescale = vec3{1_m, 1_m, 1_m};
	
	auto letterMeshes = to_array({
		"t1"_id,
		"t2"_id,
		"t3"_id,
		"t3"_id,
		"t5"_id,
		"t6"_id,
		"t7"_id,
		"t8"_id,
		"t9"_id,
		"t10"_id,
		"t11"_id,
		"t2"_id,
		"t5"_id,
		"t14"_id,
		"t15"_id,
		"t7"_id,
		"t17"_id,
		"t7"_id,
		"t2"_id,
		"t20"_id });
	
	auto letterPositions = to_array<vec3>({
		{-2.75_m, 0_m,  0.27_m},
		{-2.20_m, 0_m,  0.18_m},
		{-1.74_m, 0_m,  0.16_m},
		{-1.26_m, 0_m,  0.16_m},
		{-0.77_m, 0_m,  0.14_m},
		{-0.09_m, 0_m,  0.27_m},
		{ 0.33_m, 0_m,  0.27_m},
		{ 0.63_m, 0_m,  0.24_m},
		{ 1.02_m, 0_m,  0.24_m},
		{ 1.41_m, 0_m,  0.27_m},
		{ 1.94_m, 0_m,  0.26_m},
		{ 2.38_m, 0_m,  0.18_m},
		{ 2.83_m, 0_m,  0.14_m},
		{-1.03_m, 0_m, -0.72_m},
		{-0.45_m, 0_m, -0.79_m},
		{-0.03_m, 0_m, -0.73_m},
		{ 0.20_m, 0_m, -0.69_m},
		{ 0.43_m, 0_m, -0.73_m},
		{ 0.80_m, 0_m, -0.82_m},
		{ 1.15_m, 0_m, -0.73_m} });
	constexpr auto letterOffset = vec3{0_m, 0_m, 64_m};
	
	assert(letterMeshes.size() == letterPositions.size());
	
	auto letterIds = ivector<gfx::ObjectID>();
	defer {
		for (auto id: letterIds)
			engine.objects().destroy(id);
	};
	
	for (auto i: iota(0_zu, letterMeshes.size())) {
		
		auto obj_id = engine.objects().create();
		letterIds.emplace_back(obj_id);
		auto obj = engine.objects().get(obj_id);
		obj.meshID = letterMeshes[i];
		obj.transform.position = letterPositions[i] + letterOffset;
		obj.transform.scale = prescale;
		obj.materialID = "matte"_id;
		
		if (i >= 13 && i <= 18)
			obj.color = {1.000f, pow(f32(0xDB) / 255, 3.6), pow(f32(0xE9) / 255, 3.6), 1.0f};
		
	}
	
	auto flowerRng = Rng();
	flowerRng.seed(0xDEADBEEF);
	constexpr auto flowerOffset = vec3{0_m, 0_m, 64_m};
	auto flowerColor = vec4{
		pow(f32(0x51) / 255, 3.6),
		pow(f32(0x8A) / 255, 3.6),
		pow(f32(0x35) / 255, 3.6),
		1.0f};
	
	auto flowerBackIds = ivector<gfx::ObjectID>();
	auto flowerFrontIds = ivector<gfx::ObjectID>();
	defer {
		for (auto id: flowerBackIds)
			engine.objects().destroy(id);
		for (auto id: flowerFrontIds)
			engine.objects().destroy(id);
	};
	
	auto flowerSpins = ivector<f32>();
	auto flowerGrav = ivector<f32>();
	
	for (auto i: iota(0_zu, 512_zu)) {
		
		auto position = vec3{
			flowerRng.randFloat() * 16_m - 8_m,
			flowerRng.randFloat() * 8_m - 2_m,
			flowerRng.randFloat() * 4_m + 12_m};
		
		auto back_id = engine.objects().create();
		flowerBackIds.emplace_back(back_id);
		auto back = engine.objects().get(back_id);
		back.meshID = "flower_back"_id;
		back.transform.position = flowerOffset + position;
		back.transform.scale = prescale / 16.0f;
		back.color = flowerColor;
		back.materialID = "matte"_id;
		
		auto hue = flowerRng.randFloat() * 2.0f - 1.0f;
		hue = hue * hue * hue * hue * hue * hue;
		hue = hue / 2.0f + 0.5f;
		hue *= 60.0f;
		hue += 210.0f + flowerRng.randInt(5) * 60.0f;
		if (hue > 360.0f)
			hue -= 360.0f;
		auto frontColor = HSVtoRGB({
			hue,
			flowerRng.randFloat() * 0.4f + 0.3f,
			1.0f });
		
		auto front_id = engine.objects().create();
		flowerFrontIds.emplace_back(front_id);
		auto front = engine.objects().get(front_id);
		front.meshID = "flower_front"_id;
		front.transform.position = flowerOffset + position;
		front.transform.scale = prescale / 12.0f;
		front.color = vec4{ pow(frontColor.r(), 2.2f), pow(frontColor.g(), 2.2f), pow(frontColor.b(), 2.2f), 1.0f };
		front.materialID = "matte"_id;
		
		flowerSpins.emplace_back(flowerRng.randFloat() * 75_deg + 15_deg);
		flowerGrav.emplace_back(flowerRng.randFloat() * 6_cm + 2_cm);
		
	}
	
	L_INFO("Game initialized");
	
	// Main loop
	
	auto nextUpdate = sys::System::getTime();
	
	while (!sys::System::isQuitting()) {
		
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
		
		/*if (camMoving)
			engine.camera().rotate(camOffset.x(), camOffset.y());
		
		engine.camera().roam({
			float(camRight) - float(camLeft),
			0.0f,
			float(camUp) - float(camDown)});
		engine.camera().shift({0.0f, 0.0f, float(camFloat)});*/
		
		camOffset = vec2(0.0f);
		
		// Graphics
		
		{
			
			ZoneScopedN("Animating objects");
			constexpr auto fullSpin = 360_deg;
			constexpr auto letterPrerotate = quat::angleAxis(90_deg, {1.0f, 0.0f, 0.0f});
			
			for (auto i: iota(0_zu, letterIds.size())) {
				
				auto id = letterIds[i];
				auto progress = ratio(sys::System::getTime(), 4_s);
				progress -= f32(i) * 0.04f;
				auto progressFrac = progress - floor(progress);
				progressFrac = 1.0f - progressFrac;
				progressFrac = progressFrac * progressFrac;
				progressFrac = 1.0f - progressFrac;
				progressFrac = elasticEaseInOut(progressFrac);
				auto spin = fullSpin * progressFrac;
				auto letterRotateAnim = quat::angleAxis(spin, {0.0f, 0.0f, 1.0f});
				
				engine.objects().get(id).transform.rotation = letterRotateAnim * letterPrerotate;
				
			}
			
			constexpr auto flowerPrerotate = quat::angleAxis(90_deg, {1.0f, 0.0f, 0.0f});
			
			for (auto i: iota(0_zu, flowerBackIds.size())) {
				
				auto backId = flowerBackIds[i];
				auto frontId = flowerFrontIds[i];
				
				auto back = engine.objects().get(backId);
				auto front = engine.objects().get(frontId);
				
				auto progress = ratio(sys::System::getTime(), 4_s);
				auto backProgress = ratio(sys::System::getTime(), 3_s);
				auto frontRotateAnim = quat::angleAxis(progress * fullSpin * flowerSpins[i], {0.0f, 1.0f, 0.0f});
				auto backRotateAnim = quat::angleAxis(backProgress * fullSpin / 4.0f, {0.0f, 1.0f, 0.0f});
				
				back.transform.rotation = backRotateAnim * flowerPrerotate;
				front.transform.rotation = frontRotateAnim * flowerPrerotate;
				
				back.transform.position.z() -= flowerGrav[i];
				if (back.transform.position.z() < -4_m + flowerOffset.z())
					back.transform.position.z() += 8_m;
				
				front.transform.position.z() -= flowerGrav[i];
				if (front.transform.position.z() < -4_m + flowerOffset.z())
					front.transform.position.z() += 8_m;
				
			}
			
		}
		
		engine.render();
		
	}
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on game thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	sys::System::postQuitEvent();
	
}

}
