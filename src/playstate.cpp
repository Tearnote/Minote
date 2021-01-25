#include "playstate.hpp"

#include <algorithm>
#include <numeric>
#include <ctime>
#include "base/assert.hpp"
#include "base/util.hpp"
#include "base/math.hpp"

namespace minote {

using namespace base;
namespace ranges = std::ranges;

PlayState::PlayState():
	player{} {
	rng.seed(static_cast<u64>(std::time(nullptr)));
	ranges::fill(player.tokens, StartingTokens);
	do {
		player.preview = getRandomPiece(player);
	}
	while (player.preview == Mino4::O || player.preview == Mino4::S || player.preview == Mino4::Z);
	spawnPlayer(player);
}

void PlayState::tick(std::span<Action const> actions) {
	for (auto const& action: actions) {
		if (action.type == Action::Type::RotCCW && action.state == Action::State::Pressed)
			player.spin = spinCounterClockwise(player.spin);
		if (action.type == Action::Type::RotCW && action.state == Action::State::Pressed)
			player.spin = spinClockwise(player.spin);
	}
	grid.set({0, 0}, Mino4::I);
}

void PlayState::draw(gfx::Engine& engine) {
	// Scene
	auto const stackHeight = grid.stackHeight();

	engine.enqueueDraw("scene_base"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = glm::mat4{1.0f},
			.tint = {1.2f, 1.2f, 1.2f, 1.0f},
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_body"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, -0.1f, 0.0f}) * make_scale({1.0f, 0.1f + stackHeight, 1.0f}),
			.tint = {1.2f, 1.2f, 1.2f, 1.0f},
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_top"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, stackHeight, 0.0f}) * make_scale({1.0f, 4.1f, 1.0f}),
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_guide"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, -0.1f, 0.0f}) * make_scale({1.0f, 4.1f + stackHeight, 1.0f}),
			.tint = {1.0f, 1.0f, 1.0f, 0.02f},
		},
	}, gfx::Material::Flat);

	// Grid
	svector<gfx::Instance, 512> blockInstances;
	for (auto x: nrange(0_zu, grid.Width))
		for (auto y: nrange(0_zu, grid.Height)) {
			auto const minoOpt = grid.get({x, y});
			if (!minoOpt) continue;
			auto const mino = minoOpt.value();

			auto const translation = make_translate({
				static_cast<f32>(x) - grid.Width / 2,
				y,
				-1.0f,
			});
			blockInstances.emplace_back(gfx::Instance{
				.transform = translation,
				.tint = minoColor(mino),
			});
		}

	// Player
	auto const pieceTranslation = make_translate({
		player.position.x - i32(grid.Width / 2),
		player.position.y,
		0.0f,
	});
	auto const pieceRotationPre = make_translate({-0.5f, -0.5f, 0.0f});
	auto const pieceRotation = make_rotate(+player.spin * glm::radians(90.0f), {0.0f, 0.0f, 1.0f});
	auto const pieceRotationPost = make_translate({0.5f, 0.5f, -1.0f});
	auto const pieceTransform = pieceTranslation * pieceRotationPost * pieceRotation * pieceRotationPre;

	for (auto const block: getPiece(player.piece)) {
		auto const blockTransform = make_translate({block.x, block.y, 0.0f});
		blockInstances.emplace_back(gfx::Instance{
			.transform = pieceTransform * blockTransform,
			.tint = minoColor(player.piece),
		});
	}

	// Submit
	engine.enqueueDraw("block"_id, "opaque"_id, blockInstances,
		gfx::Material::Phong, gfx::MaterialData{.phong = {
			.ambient = 0.2f,
			.diffuse = 0.9f,
			.specular = 0.4f,
			.shine = 24.0f,
		}}
	);
}

auto PlayState::getRandomPiece(Player& p) -> Mino4 {
	auto& tokens = p.tokens;

	// Count the number of tokens
	auto const tokenTotal = std::accumulate(tokens.begin(), tokens.end(), size_t{0},
		[](auto sum, auto val) {
			val = std::max(i8{0}, val);
			return sum + val;
		});
	ASSERT(tokenTotal);

	// Create and fill the token list
	std::vector<Mino4> tokenList;
	tokenList.reserve(tokenTotal);
	for (auto i: nrange(0_zu, tokens.size())) {
		auto const count = tokens[i];
		if (count > 0)
			tokenList.insert(tokenList.end(), count, Mino4{i});
	}
	ASSERT(tokenList.size() == tokenTotal);

	// Pick a random token from the list and update the token distribution
	auto const picked = tokenList[rng.randInt(tokenTotal)];
	for (auto i: nrange(0_zu, tokens.size())) {
		if (Mino4{i} == picked)
			tokens[i] -= tokens.size() - 1;
		else
			tokens[i] += 1;
	}

	return picked;
}

void PlayState::spawnPlayer(Player& p) {
	p.position = PlayerSpawnPosition;
	p.position.y += grid.stackHeight();
	p.spin = Spin::_0;
	p.piece = p.preview;
	p.preview = getRandomPiece(p);

}

}
