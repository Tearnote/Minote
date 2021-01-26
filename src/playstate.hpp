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
	using Button = Action::Type;

	PlayState();
	void tick(std::span<Action const> actions);
	void draw(gfx::Engine& engine);

private:

	static constexpr auto StartingTokens = 6;
	static constexpr auto PlayerSpawnPosition = glm::ivec2{4, 5};

	struct Player {

		std::array<bool, +Button::Count> pressed;
		std::array<bool, +Button::Count> held;

		std::array<i8, +Mino4::ShapeCount> tokens;
		Mino4 preview;

		glm::ivec2 position;
		Mino4 piece;
		Spin spin;

	};

	Rng rng;

	Grid<10, 20> grid;
	Player p1;

	auto getRandomPiece(Player&) -> Mino4;

	void spawnPlayer(Player&);

};

}
