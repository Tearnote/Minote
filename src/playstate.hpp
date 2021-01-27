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
	static constexpr auto AutoshiftTargetInitial = 24;
	static constexpr auto AutoshiftTargetDecrement = 0.5f;

	struct Player {

		std::array<bool, +Button::Count> pressed;
		std::array<bool, +Button::Count> held;
		i32 lastDirection;

		std::array<i8, +Mino4::ShapeCount> tokens;
		Mino4 preview;

		glm::ivec2 position;
		Mino4 pieceType;
		Spin spin;

		i32 autoshift;
		i32 autoshiftTarget;
		i32 autoshiftDirection;

	};

	Rng rng;

	Grid<10, 20> grid;
	Player p1;

	auto getRandomPiece(Player&) -> Mino4;

	void spawnPlayer(Player&);

	void rotate(i32 direction);
	void shift(i32 direction);

	void updateActions(std::span<Action const>& actions);
	void updateRotation();
	void updateShift();
};

}
