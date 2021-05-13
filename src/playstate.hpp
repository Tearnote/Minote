#pragma once

#include <array>
#include <span>
#include "base/rng.hpp"
#include "gfx/engine.hpp"
#include "mapper.hpp"
#include "mino.hpp"

namespace minote {

struct PlayState {

	using Action = Mapper::Action;
	using Button = Action::Type;

	PlayState();
	void tick(std::span<Action const> actions);
	void draw(gfx::Engine& engine);

private:

	static constexpr auto StartingTokens = 6;
	static constexpr auto PlayerSpawnPosition = ivec2(4, 5);
	static constexpr auto AutoshiftTargetInitial = 24;
	static constexpr auto AutoshiftTargetMinimum = 2;
	static constexpr auto AutoshiftTargetFactor = 0.5f;
	static constexpr auto SpawnDelayTarget = 50;
	static constexpr auto DeadlineDepth = 16;

	struct Player {

		enum struct State {
			None,
			Active,
			Respawning,
		};

		std::array<bool, +Button::Count> pressed;
		std::array<bool, +Button::Count> held;
		i32 lastDirection;

		std::array<i8, +Mino4::ShapeCount> tokens;
		Mino4 preview;

		State state;

		ivec2 position;
		Mino4 pieceType;
		Spin spin;

		i32 autoshift;
		i32 autoshiftTarget;
		i32 autoshiftDirection;
		i32 spawnDelay;

	};

	Rng rng;

	Grid<10, 20> grid;
	Player p1;

	auto getRandomPiece() -> Mino4;

	void spawnPlayer();

	void rotate(i32 direction);
	void shift(i32 direction);

	void updateActions(std::span<Action const>& actions);
	void updateRotation();
	void updateShift();
	void updateSpawn();
	void updateGravity();
	void updateDeadline();
};

}
