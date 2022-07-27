#include "game.hpp"

#include "config.hpp"

#include <exception>
#include "util/vector.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "util/log.hpp"
#include "gfx/renderer.hpp"
#include "gfx/models.hpp"
#include "freecam.hpp"
#include "assets.hpp"
#include "scenes.hpp"
#include "main.hpp"

namespace minote {

struct Game::Impl {
	
	// Rate of the logic update clock. Can be higher than display refresh rate.
	static constexpr auto LogicRate = 120;
	static constexpr auto LogicTick = 1_s / LogicRate;
	
	static constexpr auto BattleScenes = 1u;
	static constexpr auto BattleSpacing = 80_m;
	static constexpr auto SimpleScenes = 1u;
	static constexpr auto SimpleSpacing = 25_m;
	
	static constexpr auto BattleSceneCount = BattleScenes * BattleScenes;
	static constexpr auto SimpleSceneCount = SimpleScenes * SimpleScenes;
	
	Impl(Game::Params const& p): m_window(p.window), m_mapper(p.mapper) {}
	// Load all assets from the store, upload to GPU when neccessary
	void loadAssets(string_view path);
	// Create game world objects, initialize camera
	void createScene();
	// Run the game logic and rendering until user quits
	void gameLoop();
	// Run a single tick of input handling and logic simulation
	// Simulation time advances to "until" timestamp
	void tick(nsec until, Imgui::InputReader&);
	
	Window& m_window;
	Mapper& m_mapper;
	
	Freecam m_freecam;
	
	svector<BattleScene, BattleSceneCount> m_battleScenes;
	svector<SimpleScene, SimpleSceneCount> m_testScenes;
	
};

void Game::Impl::loadAssets(string_view _path) {
	
	auto modelList = ModelList();
	auto assets = Assets(_path);
	assets.loadModels([&modelList](auto name, auto data) {
		modelList.addModel(name, data);
	});
	// s_renderer->uploadModels(std::move(modelList));
	
}

void Game::Impl::createScene() {
	
	s_renderer->camera() = Camera{
		.position = {8.57_m, -16.07_m, 69.20_m},
		.yaw = 2.41412449f,
		.pitch = 0.113862038f,
		.lookSpeed = 1.0f / 256.0f,
		.moveSpeed = 1_m / 16.0f,
	};
	
	constexpr auto Prescale = vec3{1_m, 1_m, 1_m};
	
	m_battleScenes.clear();
	for (auto x: iota(0u, BattleScenes))
	for (auto y: iota(0u, BattleScenes)) {
		m_battleScenes.emplace_back(Transform{
			.position = vec3{x * BattleSpacing, y * BattleSpacing, 64_m},
			.scale = Prescale,
		});
	}
	
	m_testScenes.clear();
	for (auto x: iota(0u, SimpleScenes))
	for (auto y: iota(0u, SimpleScenes)) {
		m_testScenes.emplace_back(Transform{
			.position = vec3{x * SimpleSpacing, y * SimpleSpacing, 32_m},
			.scale = Prescale,
		});
	}
	
}

void Game::Impl::gameLoop() {
	
	auto nextUpdate = s_system->getTime();
	while (!s_system->isQuitting()) {
		
		auto imguiInput = s_renderer->imgui().getInputReader();
		while (nextUpdate <= s_system->getTime()) {
			tick(nextUpdate, imguiInput);
			nextUpdate += LogicTick;
		}
		s_renderer->imgui().begin();
		m_freecam.updateCamera();
		s_renderer->render();
		
	}
	
}

void Game::Impl::tick(nsec _until, Imgui::InputReader& _imguiInput) {
	
	// Handle all relevant inputs
	s_system->forEachEvent([&] (const SDL_Event& e) -> bool {
		
		// Don't handle events from the future
		if (milliseconds(e.common.timestamp) > _until) return false;
		// Leave quit events alone
		if (e.type == SDL_QUIT) return false;
		// Let ImGui handle all events it uses
		if (_imguiInput.process(e)) return true;
		
		m_freecam.handleMouse(e);
		
		// Game logic events
		if (auto action = m_mapper.convert(e)) {
			if (action->type == Mapper::Action::Type::Back)
				s_system->postQuitEvent();
			m_freecam.handleAction(*action);
		}
		
		return true;
		
	});
	
}

Game::Game(Params const& _p): impl(new Impl(_p)) {}
Game::~Game() = default;

void Game::run() try {
	
	impl->loadAssets(Assets_p);
	impl->createScene();
	L_INFO("Game initialized");
	
	impl->gameLoop();
	
} catch (std::exception const& e) {
	
	L_CRIT("Unhandled exception on game thread: {}", e.what());
	L_CRIT("Cannot recover, shutting down. Please report this error to the developer");
	s_system->postQuitEvent();
	
}

}
