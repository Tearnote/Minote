/**
 * Implementation of mrsdraw.h
 * @file
 */

#include "mrsdraw.hpp"

#include "sys/glfw.hpp"
#include "engine/engine.hpp"
#include "particles.hpp"
#include "mrsdef.hpp"
#include "debug.hpp"
#include "mrs.hpp"

using namespace minote;

static constexpr size_t MaxBlocks = 512;
static constexpr size_t MaxBorders = 1024;

static svector<ModelPhong::Instance, MaxBlocks> opaqueBlocks{};
static svector<ModelPhong::Instance, MaxBlocks> transparentBlocks{};
static svector<ModelFlat::Instance, MaxBorders> borders{};

/// Last player position as seen by the drawing system
static ivec2 lastPlayerPos = {0, 0};

/// Last number of player piece 90-degree turns as seen by the drawing system
static int lastPlayerRotation = 0;

/// Tweening of player piece position
static Tween playerPosX = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = exponentialEaseOut
};

static Tween playerPosY = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = exponentialEaseOut
};

/// Tweening of player piece rotation
static Tween playerRotation = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = exponentialEaseOut
};

/// Player piece animation after the piece locks
static Tween lockFlash = {
	.from = 1.0f,
	.to = 0.0f,
	.duration = 8 * MrsUpdateTick,
	.type = linearInterpolation
};

/// Player piece animation as the lock delay ticks down
static Tween lockDim = {
	.from = 1.0f,
	.to = 0.1f,
	.duration = MrsLockDelay * MrsUpdateTick,
	.type = quadraticEaseIn
};

/// Animation of the scene when combo counter changes
static Tween comboFade = {
	.from = 1.1f,
	.to = 1.1f,
	.duration = 24 * MrsUpdateTick,
	.type = quadraticEaseOut
};

/// Thump animation of a falling stack
static Tween clearFall = {
	.from = 0.0f,
	.to = 1.0f,
	.duration = MrsClearDelay * MrsUpdateTick,
	.type = cubicEaseIn
};

/// Sparks released on line clear
static ParticleParams particlesClear = {
	.color = {0.0f, 0.0f, 0.0f, 1.0f}, // runtime
	.durationMin = 0_s,
	.durationMax = 1.5_s,
	.distanceMin = 3.2f, // runtime
	.distanceMax = 6.4f, // runtime
	.spinMin = 0.001f,
	.spinMax = 0.3f,
	.directionVert = 0,
	.directionHorz = 0,
	.ease = quarticEaseOut
};

/// Cloud of dust caused by a player piece falling on the stack
/// or finishing line clear thump
static ParticleParams particlesThump = {
	.color = {0.6f, 0.6f, 0.6f, 0.8f},
	.durationMin = 0.4_s,
	.durationMax = 0.8_s,
	.distanceMin = 0.2f,
	.distanceMax = 1.2f,
	.spinMin = 0.4f,
	.spinMax = 1.6f,
	.directionVert = 1,
	.directionHorz = 0,
	.ease = exponentialEaseOut
};

/// Sparks of a player piece being shifted across the playfield
static ParticleParams particlesSlide = {
	.color = {0.0f, 0.4f, 2.0f, 1.0f},
	.durationMin = 0.3_s,
	.durationMax = 0.6_s,
	.distanceMin = 0.2f,
	.distanceMax = 1.4f,
	.spinMin = 0.4f,
	.spinMax = 1.2f,
	.directionVert = 1,
	.directionHorz = 0, // runtime
	.ease = exponentialEaseOut
};

/// Sparks of a player piece being DASed across the playfield
static ParticleParams particlesSlideFast = {
	.color = {2.0f, 0.4f, 0.0f, 1.0f},
	.durationMin = 0.25_s,
	.durationMax = 0.5_s,
	.distanceMin = 0.4f,
	.distanceMax = 2.0f,
	.spinMin = 0.4f,
	.spinMax = 1.2f,
	.directionVert = 1,
	.directionHorz = 0, // runtime
	.ease = exponentialEaseOut
};

/**
 * Create a dust cloud effect under the player piece.
 */
static void mrsEffectDrop(void)
{
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = mrsTet.player.pos.x + mrsTet.player.shape[i].x;
		int y = mrsTet.player.pos.y + mrsTet.player.shape[i].y;
		if (fieldGet(mrsTet.field, (ivec2){x, y - 1})) {
			particlesGenerate((vec3){
				(f32)x - (f32)FieldWidth / 2,
				(f32)y,
				0.0f
			}, 8, &particlesThump);
		}
	}
}

static void mrsQueueBorder(vec3 pos, vec3 size, color4 color)
{
	mat4 const transformTemp = make_translate<>(pos);
	borders.push_back({
		.tint = color,
		.transform = scale(transformTemp, size)
	});
}

static void mrsDebug(void)
{
	if (nk_begin(nkCtx(), "MRS debug", nk_rect(30, 30, 200, 180),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE
			| NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(nkCtx(), 0, 2);
		nk_labelf(nkCtx(), NK_TEXT_CENTERED, "Gravity: %d.%02x",
			mrsTet.player.gravity / MrsSubGrid,
			mrsTet.player.gravity % MrsSubGrid);
		nk_slider_int(nkCtx(), 4, &mrsTet.player.gravity, MrsSubGrid * 20, 4);
		nk_layout_row_dynamic(nkCtx(), 0, 1);
		nk_checkbox_label(nkCtx(), "Pause spawning", &mrsDebugPauseSpawn);
		nk_checkbox_label(nkCtx(), "Infinite lock delay", &mrsDebugInfLock);
		if (nk_button_label(nkCtx(), "Restart game")) {
			mrsCleanup();
			mrsInit();
		}
	}
	nk_end(nkCtx());

	if (nk_begin(nkCtx(), "MRS playfield", nk_rect(30, 250, 200, 440),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE
			| NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(nkCtx(), 16, 10);
		for (int y = MrsFieldHeightVisible - 1; y >= 0; y -= 1) {
			for (int x = 0; x < FieldWidth; x += 1) {
				mino cell = fieldGet(mrsTet.field, (ivec2){x, y});
				color4 cellColor = minoColor(cell);
				if (nk_button_color(nkCtx(), nk_rgba(
					cellColor.r * 255.0f,
					cellColor.g * 255.0f,
					cellColor.b * 255.0f,
					cellColor.a * 255.0f))) {
					if (cell)
						fieldSet(mrsTet.field, (ivec2){x, y}, MinoNone);
					else
						fieldSet(mrsTet.field, (ivec2){x, y}, MinoGarbage);
				}
			}
		}
	}
	nk_end(nkCtx());
}

void mrsDraw(Engine& engine)
{
	// Draw field scene
	f32 const sceneBoost = comboFade.apply();
	engine.models.field.draw(*engine.frame.fb, engine.scene, {
		.blending = true
	}, {
		.tint = {sceneBoost, sceneBoost, sceneBoost, 1.0f}
	});

	// Draw column guide
	engine.models.guide.draw(*engine.frame.fb, engine.scene, {
		.blending = true
	});

	// Queue up blocks in the field
	int linesCleared = 0;
	f32 const fallProgress = clearFall.apply();

	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		ivec2 const pos = {i % FieldWidth, i / FieldWidth};

		if (mrsTet.linesCleared[pos.y]) {
			linesCleared += 1;
			i += FieldWidth - 1;
			continue;
		}

		mino const type = fieldGet(mrsTet.field, pos);
		if (type == MinoNone) continue;

		bool const opaque = (minoColor(type).a == 1.0f);
		auto& instances = opaque ? opaqueBlocks : transparentBlocks;
		auto& instance = instances.emplace_back();

		instance.tint = minoColor(type);
		instance.tint.r *= MrsFieldDim;
		instance.tint.g *= MrsFieldDim;
		instance.tint.b *= MrsFieldDim;
		if (pos.y >= MrsFieldHeightVisible)
			instance.tint.a *= MrsExtraRowDim;

		bool playerCell = false;
		for (size_t j = 0; j < MinosPerPiece; j += 1) {
			ivec2 const ppos = mrsTet.player.shape[j] + mrsTet.player.pos;
			if (pos == ppos) {
				playerCell = true;
				break;
			}
		}
		if (playerCell) {
			f32 const flash = lockFlash.apply();
			instance.highlight = {MrsLockFlashBrightness,
			                       MrsLockFlashBrightness,
			                       MrsLockFlashBrightness, flash};
		}

		vec2 const fpos = {
			pos.x,
			static_cast<f32>(pos.y) - static_cast<f32>(linesCleared) * fallProgress
		};
		instance.transform = make_translate({
			fpos.x - static_cast<f32>(FieldWidth / 2),
			fpos.y,
			0.0f
		});
	}

	// Queue up player piece blocks

	// Tween the player position
	if (mrsTet.player.pos.x != lastPlayerPos.x) {
		playerPosX.from = playerPosX.apply();
		playerPosX.to = mrsTet.player.pos.x;
		if (mrsTet.player.autoshiftCharge == MrsAutoshiftCharge) {
			playerPosX.duration = 1 * MrsUpdateTick;
			playerPosX.type = linearInterpolation;
		} else {
			playerPosX.duration = 3 * MrsUpdateTick;
			playerPosX.type = exponentialEaseOut;
		}
		playerPosX.restart();
		lastPlayerPos.x = mrsTet.player.pos.x;
	}
	if (mrsTet.player.pos.y != lastPlayerPos.y) {
		playerPosY.from = playerPosY.apply();
		playerPosY.to = mrsTet.player.pos.y;
		playerPosY.restart();
		lastPlayerPos.y = mrsTet.player.pos.y;
	}

	// Tween the player rotation
	if (mrsTet.player.rotation != tmod(lastPlayerRotation, +SpinSize)) {
		int delta =
			mrsTet.player.rotation - tmod(lastPlayerRotation, +SpinSize);
		if (delta == 3) delta -= 4;
		if (delta == -3) delta += 4;
		playerRotation.from = playerRotation.apply();
		lastPlayerRotation += delta;
		playerRotation.to = lastPlayerRotation;
		playerRotation.restart();
	}

	// Draw the blocks if needed
	if (mrsTet.player.state == PlayerActive
		|| mrsTet.player.state == PlayerSpawned) {
		// Get player piece shape (not rotated)
		piece player = {};
		arrayCopy(player, MrsPieces[mrsTet.player.type]);

		// Get piece transform (piece position and rotation)
		mat4 const pieceTranslation = make_translate({
			playerPosX.apply() - (signed)(FieldWidth / 2),
			playerPosY.apply(),
			0.0f
		});
		mat4 const pieceRotationPre = make_translate({0.5f, 0.5f, 0.0f});
		mat4 const pieceRotation = rotate(pieceRotationPre,
			playerRotation.apply() * radians(90.0f), {0.0f, 0.0f, 1.0f});
		mat4 const pieceRotationPost = translate(pieceRotation,
			{-0.5f, -0.5f, 0.0f});
		mat4 const pieceTransform = pieceTranslation * pieceRotationPost;

		for (size_t i = 0; i < MinosPerPiece; i += 1) {
			// Get mino transform (offset from piece origin)
			mat4 const minoTransform = make_translate(
				{player[i].x, player[i].y, 0.0f});

			// Queue up next mino
			bool const opaque = (minoColor(mrsTet.player.type).a == 1.0);
			auto& instances = opaque ? opaqueBlocks : transparentBlocks;
			auto& instance = instances.emplace_back();

			// Insert calculated values
			instance.tint = minoColor(mrsTet.player.type);
			if (mrsTet.player.lockDelay != 0) {
				lockDim.restart();
				lockDim.start -= mrsTet.player.lockDelay * MrsUpdateTick;
				f32 dim = lockDim.apply();
				instance.tint.r *= dim;
				instance.tint.g *= dim;
				instance.tint.b *= dim;
			}
			instance.transform = pieceTransform * minoTransform;
		}
	}

	// Queue up ghost piece blocks
	if ((mrsTet.player.state == PlayerActive ||
		mrsTet.player.state == PlayerSpawned) &&
		mrsTet.player.gravity < MrsSubGrid && // Don't show if the game is too fast for it to help
			(!mrsTet.player.lockDelay ||
			(Glfw::getTime() < playerPosY.start + playerPosY.duration)) // Don't show if player is on the ground
		) {
		ivec2 ghostPos = mrsTet.player.pos;
		while (!pieceOverlapsField(&mrsTet.player.shape, {
			ghostPos.x,
			ghostPos.y - 1
		}, mrsTet.field))
			ghostPos.y -= 1; // Drop down as much as possible

		for (size_t i = 0; i < MinosPerPiece; i += 1) {
			vec2 const pos = mrsTet.player.shape[i] + ghostPos;

			auto& instance = transparentBlocks.emplace_back();

			instance.tint = minoColor(mrsTet.player.type);
			instance.tint.a *= MrsGhostDim;
			instance.transform = make_translate(
				{pos.x - (signed)(FieldWidth / 2), pos.y, 0.0f});
		}
	}

	// Queue up piece preview blocks
	if (mrsTet.player.preview != MinoNone) {
		piece previewPiece = {};
		arrayCopy(previewPiece, MrsPieces[mrsTet.player.preview]);
		for (size_t i = 0; i < MinosPerPiece; i += 1) {
			vec2 pos = {
				previewPiece[i].x + MrsPreviewX,
				previewPiece[i].y + MrsPreviewY
			};
			if (mrsTet.player.preview == MinoI)
				pos.y -= 1;

			bool const opaque = (minoColor(mrsTet.player.preview).a == 1.0);
			auto& instances = opaque ? opaqueBlocks : transparentBlocks;
			auto& instance = instances.emplace_back();

			instance.tint = minoColor(mrsTet.player.preview);
			instance.transform = make_translate({pos.x, pos.y, 0.0f});
		}
	}

	// Draw all queued blocks
	engine.models.block.draw(*engine.frame.fb, engine.scene, {}, opaqueBlocks);
	opaqueBlocks.clear();
	engine.models.block.draw(*engine.frame.fb, engine.scene, {
		.colorWrite = false
	}, transparentBlocks);
	engine.models.block.draw(*engine.frame.fb, engine.scene, {
		.blending = true
	}, transparentBlocks);
	transparentBlocks.clear();

	// Draw the block borders
	linesCleared = 0;
	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		ivec2 const pos = {i % FieldWidth, i / FieldWidth};

		if (mrsTet.linesCleared[pos.y]) {
			i += FieldWidth - 1;
			linesCleared += 1;
			continue;
		}

		if (!fieldGet(mrsTet.field, pos)) continue;

		// Coords transformed to world space
		vec2 const worldPos = {
			static_cast<f32>(pos.x) - static_cast<f32>(FieldWidth / 2),
			static_cast<f32>(pos.y) - static_cast<f32>(linesCleared) * fallProgress
		};
		f32 alpha = MrsBorderDim;
		if (pos.y >= MrsFieldHeightVisible)
			alpha *= MrsExtraRowDim;

		// Left
		if (!fieldGet(mrsTet.field, {pos.x - 1, pos.y}))
			mrsQueueBorder({worldPos.x, worldPos.y + 0.125f, 0.0f},
				{0.125f, 0.75f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Right
		if (!fieldGet(mrsTet.field, {pos.x + 1, pos.y}))
			mrsQueueBorder({worldPos.x + 0.875f, worldPos.y + 0.125f, 0.0f},
				{0.125f, 0.75f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Down
		if (!fieldGet(mrsTet.field, {pos.x, pos.y - 1}))
			mrsQueueBorder({worldPos.x + 0.125f, worldPos.y, 0.0f},
				{0.75f, 0.125f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Up
		if (!fieldGet(mrsTet.field, {pos.x, pos.y + 1}))
			mrsQueueBorder({worldPos.x + 0.125f, worldPos.y + 0.875f, 0.0f},
				{0.75f, 0.125f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Down Left
		if (!fieldGet(mrsTet.field, {pos.x - 1, pos.y - 1})
			|| !fieldGet(mrsTet.field, {pos.x - 1, pos.y})
			|| !fieldGet(mrsTet.field, {pos.x, pos.y - 1}))
			mrsQueueBorder({worldPos.x, worldPos.y, 0.0f},
				{0.125f, 0.125f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Down Right
		if (!fieldGet(mrsTet.field, {pos.x + 1, pos.y - 1})
			|| !fieldGet(mrsTet.field, {pos.x + 1, pos.y})
			|| !fieldGet(mrsTet.field, {pos.x, pos.y - 1}))
			mrsQueueBorder({worldPos.x + 0.875f, worldPos.y, 0.0f},
				{0.125f, 0.125f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Up Left
		if (!fieldGet(mrsTet.field, {pos.x - 1, pos.y + 1})
			|| !fieldGet(mrsTet.field, {pos.x - 1, pos.y})
			|| !fieldGet(mrsTet.field, {pos.x, pos.y + 1}))
			mrsQueueBorder({worldPos.x, worldPos.y + 0.875f, 0.0f},
				{0.125f, 0.125f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
		// Up Right
		if (!fieldGet(mrsTet.field, {pos.x + 1, pos.y + 1})
			|| !fieldGet(mrsTet.field, {pos.x + 1, pos.y})
			|| !fieldGet(mrsTet.field, {pos.x, pos.y + 1}))
			mrsQueueBorder({worldPos.x + 0.875f, worldPos.y + 0.875f, 0.0f},
				{0.125f, 0.125f, 1.0f},
				{1.0f, 1.0f, 1.0f, alpha});
	}

	engine.models.border.draw(*engine.frame.fb, engine.scene, {
		.blending = true,
	}, borders);
	borders.clear();

#ifdef MINOTE_DEBUG
	mrsDebug();
#endif //MINOTE_DEBUG
}

void mrsEffectSpawn(void)
{
	lastPlayerPos = mrsTet.player.pos;
	lastPlayerRotation = mrsTet.player.rotation;
	playerPosX.from = lastPlayerPos.x;
	playerPosX.to = lastPlayerPos.x;
	playerPosY.from = lastPlayerPos.y + 1;
	playerPosY.to = lastPlayerPos.y;
	playerRotation.from = lastPlayerRotation;
	playerRotation.to = lastPlayerRotation;
	playerPosX.restart();
	playerPosY.restart();
	playerRotation.restart();
}

void mrsEffectLock(void)
{
	lockFlash.restart();
}

void mrsEffectClear(int row, int power)
{
	for (int x = 0; x < FieldWidth; x += 1) {
		for (int ySub = 0; ySub < 8; ySub += 1) {
			color4 cellColor = minoColor(
				fieldGet(mrsTet.field, (ivec2){x, row}));
			particlesClear.color = cellColor;
			particlesClear.color.r *= MrsParticlesClearBoost;
			particlesClear.color.g *= MrsParticlesClearBoost;
			particlesClear.color.b *= MrsParticlesClearBoost;
			particlesClear.distanceMin = 3.2f * power;
			particlesClear.distanceMax = particlesClear.distanceMin * 2.0f;
			particlesGenerate((vec3){
					(f32)x - (f32)FieldWidth / 2,
					(f32)row + 0.0625f + 0.125f * (f32)ySub,
					0.0f},
				power, &particlesClear);
		}
	}

	clearFall.restart();
}

void mrsEffectThump(int row)
{
	for (int x = 0; x < FieldWidth; x += 1) {
		if (fieldGet(mrsTet.field, (ivec2){x, row})
			&& fieldGet(mrsTet.field, (ivec2){x, row - 1}))
			particlesGenerate((vec3){
				(f32)x - (f32)FieldWidth / 2,
				(f32)row,
				0.0f
			}, 8, &particlesThump);
	}
}

void mrsEffectLand(int direction)
{
	if (direction == -1)
		mrsEffectSlide(-1,
			(mrsTet.player.autoshiftCharge == MrsAutoshiftCharge));
	else if (direction == 1)
		mrsEffectSlide(1,
			(mrsTet.player.autoshiftCharge == MrsAutoshiftCharge));
	else
		mrsEffectDrop();
}

void mrsEffectSlide(int direction, bool fast)
{
	ParticleParams* params = fast ? &particlesSlideFast : &particlesSlide;
	params->directionHorz = direction;

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = mrsTet.player.pos.x + mrsTet.player.shape[i].x;
		int y = mrsTet.player.pos.y + mrsTet.player.shape[i].y;
		if (fieldGet(mrsTet.field, (ivec2){x, y - 1})) {
			particlesGenerate((vec3){
				(f32)x - (f32)FieldWidth / 2,
				(f32)y,
				0.0f
			}, 8, params);
		}
	}
}
