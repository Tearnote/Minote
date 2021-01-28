#include "playstate.hpp"

#include <algorithm>
#include <numeric>
#include <ctime>
#include "base/assert.hpp"
#include "base/util.hpp"
#include "base/math.hpp"

#include "base/log.hpp"

namespace minote {

using namespace base;
namespace ranges = std::ranges;

PlayState::PlayState():
	p1{} {
	rng.seed(static_cast<u64>(std::time(nullptr)));
	p1.tokens.fill(StartingTokens);
	do {
		p1.preview = getRandomPiece();
	}
	while (p1.preview == Mino4::O || p1.preview == Mino4::S || p1.preview == Mino4::Z);
}

void PlayState::tick(std::span<Action const> actions) {
	updateActions(actions);
	updateRotation();
	updateShift();
	updateSpawn();
	updateGravity();
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
			.transform = make_translate({0.0f, stackHeight, 0.0f}) * make_scale({1.0f, 5.1f, 1.0f}),
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_guide"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, -0.1f, 0.0f}) * make_scale({1.0f, 5.1f + stackHeight, 1.0f}),
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
	if (p1.state == Player::State::Active) {
		auto const pieceTranslation = make_translate({
			p1.position.x - i32(grid.Width / 2),
			p1.position.y,
			0.0f,
		});
		auto const pieceRotationPre = make_translate({-0.5f, -0.5f, 0.0f});
		auto const pieceRotation = make_rotate(+p1.spin * glm::radians(90.0f), {0.0f, 0.0f, 1.0f});
		auto const pieceRotationPost = make_translate({0.5f, 0.5f, -1.0f});
		auto const pieceTransform = pieceTranslation * pieceRotationPost * pieceRotation * pieceRotationPre;

		for (auto const block: minoPiece(p1.pieceType)) {
			auto const blockTransform = make_translate({block.x, block.y, 0.0f});
			blockInstances.emplace_back(gfx::Instance{
				.transform = pieceTransform * blockTransform,
				.tint = minoColor(p1.pieceType),
			});
		}
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

auto PlayState::getRandomPiece() -> Mino4 {
	// Count the number of tokens
	auto const tokenTotal = std::accumulate(p1.tokens.begin(),p1.tokens.end(), size_t{0},
		[](auto sum, auto val) {
			val = std::max(i8{0}, val);
			return sum + val;
		});
	ASSERT(tokenTotal);

	// Create and fill the token list
	std::vector<Mino4> tokenList;
	tokenList.reserve(tokenTotal);
	for (auto i: nrange(0_zu, p1.tokens.size())) {
		auto const count = p1.tokens[i];
		if (count > 0)
			tokenList.insert(tokenList.end(), count, Mino4{i});
	}
	ASSERT(tokenList.size() == tokenTotal);

	// Pick a random token from the list and update the token distribution
	auto const picked = tokenList[rng.randInt(tokenTotal)];
	for (auto i: nrange(0_zu, p1.tokens.size())) {
		if (Mino4{i} == picked)
			p1.tokens[i] -= p1.tokens.size() - 1;
		else
			p1.tokens[i] += 1;
	}

	return picked;
}

void PlayState::spawnPlayer() {
	p1.pieceType = p1.preview;
	p1.preview = getRandomPiece();

	p1.position = PlayerSpawnPosition;
	p1.position.y += grid.stackHeight();
	p1.spin = Spin::_0;
	p1.state = Player::State::Active;
	p1.spawnDelay = 0;
}

void PlayState::rotate(i32 direction) {
	if (p1.state != Player::State::Active) return;

	auto const origSpin = p1.spin;
	auto const origPosition = p1.position;

	p1.spin = spinCounterClockwise(p1.spin, direction);

	// Apply crawl offsets to I
	if (p1.pieceType == Mino4::I) {
		if (origSpin == Spin::_0 && p1.spin == Spin::_90)
			p1.position.y -= 1;
		if (origSpin == Spin::_90 && p1.spin == Spin::_180)
			p1.position.y += 1;
		if (origSpin == Spin::_180 && p1.spin == Spin::_270)
			p1.position.x -= 1;
		if (origSpin == Spin::_270 && p1.spin == Spin::_0)
			p1.position.x -= 1;

		if (origSpin == Spin::_0 && p1.spin == Spin::_270)
			p1.position.x += 1;
		if (origSpin == Spin::_270 && p1.spin == Spin::_180)
			p1.position.x += 1;
		if (origSpin == Spin::_180 && p1.spin == Spin::_90)
			p1.position.y -= 1;
		if (origSpin == Spin::_90 && p1.spin == Spin::_0)
			p1.position.y += 1;
	}

	// Apply crawl offsets to S and Z
	if (p1.pieceType == Mino4::S || p1.pieceType == Mino4::Z) {
		if (origSpin == Spin::_0 && p1.spin == Spin::_90)
			p1.position.x -= 1;
		if (origSpin == Spin::_90 && p1.spin == Spin::_180)
			p1.position.y -= 1;
		if (origSpin == Spin::_180 && p1.spin == Spin::_270)
			p1.position.y += 1;
		if (origSpin == Spin::_270 && p1.spin == Spin::_0)
			p1.position.x -= 1;

		if (origSpin == Spin::_0 && p1.spin == Spin::_270)
			p1.position.x += 1;
		if (origSpin == Spin::_270 && p1.spin == Spin::_180)
			p1.position.y -= 1;
		if (origSpin == Spin::_180 && p1.spin == Spin::_90)
			p1.position.y += 1;
		if (origSpin == Spin::_90 && p1.spin == Spin::_0)
			p1.position.x += 1;
	}

	// Keep O in place
	if (p1.pieceType == Mino4::O) {
		if (origSpin == Spin::_0 && p1.spin == Spin::_90)
			p1.position.y -= 1;
		if (origSpin == Spin::_90 && p1.spin == Spin::_180)
			p1.position.x += 1;
		if (origSpin == Spin::_180 && p1.spin == Spin::_270)
			p1.position.y += 1;
		if (origSpin == Spin::_270 && p1.spin == Spin::_0)
			p1.position.x -= 1;

		if (origSpin == Spin::_0 && p1.spin == Spin::_270)
			p1.position.x += 1;
		if (origSpin == Spin::_270 && p1.spin == Spin::_180)
			p1.position.y -= 1;
		if (origSpin == Spin::_180 && p1.spin == Spin::_90)
			p1.position.x -= 1;
		if (origSpin == Spin::_90 && p1.spin == Spin::_0)
			p1.position.y += 1;
	}

	// Check if any kick succeeds
	auto const successful = [this] {
		auto check = [this] {
			return !grid.overlaps(p1.position, minoPiece(p1.pieceType, p1.spin));
		};

		// Base position
		if (check()) return true;

		// I doesn't kick
		if (p1.pieceType == Mino4::I) return false;

		// Down
		p1.position.y -= 1;
		if(check()) return true;
		p1.position.y += 1;

		// Left/right
		p1.position.x += p1.lastDirection;
		if(check()) return true;
		p1.position.x -= p1.lastDirection * 2;
		if(check()) return true;
		p1.position.x += p1.lastDirection;

		// Down + left/right
		p1.position.y -= 1;
		p1.position.x += p1.lastDirection;
		if(check()) return true;
		p1.position.x -= p1.lastDirection * 2;
		if(check()) return true;
		p1.position.x += p1.lastDirection;
		p1.position.y += 1;

		// Failure
		return false;
	}();

	// Restore original state if failed
	if (!successful) {
		p1.spin = origSpin;
		p1.position = origPosition;
	}
}

void PlayState::shift(i32 direction) {
	if (p1.state != Player::State::Active) return;
	auto const origPosition = p1.position;

	p1.position.x += direction;

	if (grid.overlaps(p1.position, minoPiece(p1.pieceType, p1.spin)))
		p1.position = origPosition;
}

void PlayState::updateActions(std::span<Action const>& actions) {// Collect player actions
	p1.pressed.fill(false);
	for (auto const& action: actions) {
		auto const state = (action.state == Action::State::Pressed)? true : false;
		p1.pressed[+action.type] = state;
		p1.held[+action.type] = state;
	}

	// Filter player actions
	if (p1.held[+Button::Left] && p1.pressed[+Button::Right])
		p1.held[+Button::Left] = false;
	if (p1.held[+Button::Right] && p1.pressed[+Button::Left])
		p1.held[+Button::Right] = false;

	// Update last direction
	if (p1.pressed[+Button::Left])
		p1.lastDirection = -1;
	if (p1.pressed[+Button::Right] || p1.lastDirection == 0)
		p1.lastDirection = 1;
}

void PlayState::updateRotation() {
	if (p1.pressed[+Button::RotCW]) rotate(-1);
	if (p1.pressed[+Button::RotCCW]) rotate(1);
	if (p1.pressed[+Button::RotCCW2]) rotate(1);

}

void PlayState::updateShift() {
	// Update autoshift
	if ((p1.held[+Button::Left] && p1.autoshiftDirection == -1) ||
		(p1.held[+Button::Right] && p1.autoshiftDirection == 1)) {
		p1.autoshift += 1;
	} else {
		p1.autoshift = 0;
		p1.autoshiftTarget = AutoshiftTargetInitial;
		if (p1.held[+Button::Left])
			p1.autoshiftDirection = -1;
		else if (p1.held[+Button::Right])
			p1.autoshiftDirection = 1;
		else
			p1.autoshiftDirection = 0;
	}

	// Execute direct shift
	if (p1.pressed[+Button::Left]) shift(-1);
	if (p1.pressed[+Button::Right]) shift(1);

	// Execute autoshift
	if (p1.autoshift == p1.autoshiftTarget) {
		shift(p1.autoshiftDirection);
		p1.autoshift = 0;
		if (p1.state == Player::State::Active) // Live piece, use normal autoshift multiplier
			p1.autoshiftTarget = std::ceil(float(p1.autoshiftTarget) * AutoshiftTargetFactor);
		else // Otherwise, precharge instantly
			p1.autoshiftTarget = 1;
	}
}

void PlayState::updateSpawn() {
	// Initial spawn
	if (p1.state == Player::State::None)
		spawnPlayer();

	// Respawn
	if (p1.state == Player::State::Respawning) {
		p1.spawnDelay += 1;
		if (p1.spawnDelay == SpawnDelayTarget)
			spawnPlayer();
	}
}

void PlayState::updateGravity() {
	if (p1.state != Player::State::Active) return;

	// Drop to the lowest position
	if (p1.held[+Button::Drop] || p1.held[+Button::Lock]) {
		while (!grid.overlaps(p1.position, minoPiece(p1.pieceType, p1.spin)))
			p1.position.y -= 1;
		p1.position.y += 1;
	}

	// Lock the piece
	if (p1.held[+Button::Lock]) {
		grid.stamp(p1.position, minoPiece(p1.pieceType, p1.spin), p1.pieceType);
		p1.state = Player::State::Respawning;
	}
}

}
