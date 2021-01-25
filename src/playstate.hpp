#pragma once

#include <array>
#include <span>
#include "base/rng.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
#include "mino.hpp"

namespace minote {

struct PlayState {

	using Action = engine::Mapper::Action;

	PlayState();
	void tick(std::span<Action const> actions);
	void draw(gfx::Engine& engine);

private:

	static constexpr auto StartingTokens = 6;
	static constexpr auto PlayerSpawnPosition = glm::ivec2{4, 5};

	struct Player {

		std::array<i8, 7> tokens;
		Mino4 piece;
		Mino4 preview;
		Spin spin;
		glm::ivec2 position;

	};

	Rng rng;

	Grid<10, 20> grid;
	Player player;

	auto getRandomPiece(Player&) -> Mino4;

	void spawnPlayer(Player&);

};

}
