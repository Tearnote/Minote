/**
 * Implementation of mrsdraw.h
 * @file
 */

#include "mrsdraw.hpp"

#include "particles.hpp"
#include "mrsdef.hpp"
#include "debug.hpp"
#include "model.hpp"
#include "world.hpp"
#include "mrs.hpp"

using namespace minote;

static constexpr std::size_t BlocksMax{512};
static constexpr std::size_t BordersMax{1024};

static Model* scene = nullptr;
static Model* guide = nullptr;

static Model* block = nullptr;
static varray<color4, BlocksMax> blockTintsOpaque{};
static varray<color4, BlocksMax> blockHighlightsOpaque{};
static varray<mat4, BlocksMax> blockTransformsOpaque{};
static varray<color4, BlocksMax> blockTintsAlpha{};
static varray<color4, BlocksMax> blockHighlightsAlpha{};
static varray<mat4, BlocksMax> blockTransformsAlpha{};

static Model* border = nullptr;
static varray<color4, BordersMax> borderTints{};
static varray<mat4, BordersMax> borderTransforms{};

static bool initialized = false;

/// Last player position as seen by the drawing system
static ivec2 lastPlayerPos = {0, 0};

/// Last number of player piece 90-degree turns as seen by the drawing system
static int lastPlayerRotation = 0;

/// Tweening of player piece position
static Tween<float> playerPosX = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = exponentialEaseOut
};

static Tween<float> playerPosY = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = exponentialEaseOut
};

/// Tweening of player piece rotation
static Tween<float> playerRotation = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = exponentialEaseOut
};

/// Player piece animation after the piece locks
static Tween<float> lockFlash = {
	.from = 1.0f,
	.to = 0.0f,
	.duration = 8 * MrsUpdateTick,
	.type = linearInterpolation
};

/// Player piece animation as the lock delay ticks down
static Tween<float> lockDim = {
	.from = 1.0f,
	.to = 0.1f,
	.duration = MrsLockDelay * MrsUpdateTick,
	.type = quadraticEaseIn
};

/// Animation of the scene when combo counter changes
static Tween<float> comboFade = {
	.from = 1.1f,
	.to = 1.1f,
	.duration = 24 * MrsUpdateTick,
	.type = quadraticEaseOut
};

/// Thump animation of a falling stack
static Tween<float> clearFall = {
	.from = 0.0f,
	.to = 1.0f,
	.duration = MrsClearDelay * MrsUpdateTick,
	.type = cubicEaseIn
};

/// Sparks released on line clear
static ParticleParams particlesClear = {
	.color = {0.0f, 0.0f, 0.0f, 1.0f}, // runtime
	.durationMin = seconds(0),
	.durationMax = seconds(1.5),
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
	.durationMin = seconds(0.4),
	.durationMax = seconds(0.8),
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
	.durationMin = seconds(0.3),
	.durationMax = seconds(0.6),
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
	.durationMin = seconds(0.25),
	.durationMax = seconds(0.5),
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
				(float)x - (float)FieldWidth / 2,
				(float)y,
				0.0f
			}, 8, &particlesThump);
		}
	}
}

/**
 * Draw the scene model, which visually wraps the tetrion field.
 */
static void mrsDrawScene(void)
{
	float boost = comboFade.apply();
	color4 boostColor[] = {{boost, boost, boost, 1.0f}};
	auto identity = mat4(1.0f);
	modelDraw(scene, 1, boostColor, nullptr, &identity);
}

/**
 * Draw the guide model, helping a beginner player keep track of columns.
 */
static void mrsDrawGuide(void)
{
	color4 white[]{{1.0f, 1.0f, 1.0f, 1.0f}};
	auto identity = mat4(1.0f);
	modelDraw(guide, 1, white, nullptr, &identity);
}

/**
 * Queue the contents of the tetrion field.
 */
static void mrsQueueField(void)
{
	int linesCleared = 0;
	float fallProgress = clearFall.apply();

	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;

		if (mrsTet.linesCleared[y]) {
			linesCleared += 1;
			i += FieldWidth - 1;
			continue;
		}

		mino type = fieldGet(mrsTet.field, (ivec2){x, y});
		if (type == MinoNone) continue;

		const bool transparent = (minoColor(type).a == 1.0);
		auto& tints = transparent? blockTintsOpaque : blockTintsAlpha;
		auto& highlights = transparent? blockHighlightsOpaque : blockHighlightsAlpha;
		auto& transforms = transparent? blockTransformsOpaque : blockTransformsAlpha;
		color4* const tint = tints.produce();
		color4* const highlight = highlights.produce();
		mat4* const transform = transforms.produce();
		if (!tint)
			return; // Block limit reached, no point continuing
		ASSERT(highlight);
		ASSERT(transform);

		*tint = minoColor(type);
		tint->r *= MrsFieldDim;
		tint->g *= MrsFieldDim;
		tint->b *= MrsFieldDim;
		if (y >= MrsFieldHeightVisible)
			tint->a *= MrsExtraRowDim;

		bool playerCell = false;
		for (size_t j = 0; j < MinosPerPiece; j += 1) {
			int px = mrsTet.player.shape[j].x + mrsTet.player.pos.x;
			int py = mrsTet.player.shape[j].y + mrsTet.player.pos.y;
			if (x == px && y == py) {
				playerCell = true;
				break;
			}
		}
		if (playerCell) {
			float flash = lockFlash.apply();
			*highlight = ((color4){MrsLockFlashBrightness, MrsLockFlashBrightness,
				          MrsLockFlashBrightness, flash});
		} else {
			*highlight = {1.0f, 1.0f, 1.0f, 0.0f};
		}

		float fx = (float)x;
		float fy = (float)y - (float)linesCleared * fallProgress;
		*transform = translate(mat4(1.0f), {
			fx - (signed)(FieldWidth / 2),
			fy,
			0.0f
		});
	}
}

/**
 * Queue the player piece on top of the field.
 */
static void mrsQueuePlayer(void)
{
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
		int delta = mrsTet.player.rotation - tmod(lastPlayerRotation, +SpinSize);
		if (delta == 3) delta -= 4;
		if (delta == -3) delta += 4;
		playerRotation.from = playerRotation.apply();
		lastPlayerRotation += delta;
		playerRotation.to = lastPlayerRotation;
		playerRotation.restart();
	}

	// Stop if no drawing needed
	if (mrsTet.player.state != PlayerActive &&
		mrsTet.player.state != PlayerSpawned)
		return;

	// Get player piece shape (not rotated)
	piece player = {};
	arrayCopy(player, MrsPieces[mrsTet.player.type]);

	// Get piece transform (piece position and rotation)
	const mat4 pieceTranslation = translate(mat4(1.0f), {
		playerPosX.apply() - (signed)(FieldWidth / 2),
		playerPosY.apply(),
		0.0f
	});
	const mat4 pieceRotationPre = translate(mat4(1.0f), {
		0.5f,
		0.5f,
		0.0f
	});
	const mat4 pieceRotation = rotate(pieceRotationPre,
		playerRotation.apply() * radians(90.0f), {0.0f, 0.0f, 1.0f});
//	mat4x4_rotate_Z(pieceRotation, pieceRotationTemp,
//		playerRotation.apply() * rad(90.0f));
	const mat4 pieceRotationPost = translate(pieceRotation, {
		-0.5f,
		-0.5f,
		0.0f
	});
	const mat4 pieceTransform = pieceTranslation * pieceRotationPost;

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		// Get mino transform (offset from piece origin)
		mat4 minoTransform = translate(mat4(1.0f), {
			player[i].x,
			player[i].y,
			0.0f
		});

		// Queue up next mino
		const bool transparent = (minoColor(mrsTet.player.type).a == 1.0);
		auto& tints = transparent? blockTintsOpaque : blockTintsAlpha;
		auto& highlights = transparent? blockHighlightsOpaque : blockHighlightsAlpha;
		auto& transforms = transparent? blockTransformsOpaque : blockTransformsAlpha;
		auto* const tint = tints.produce();
		auto* const highlight = highlights.produce();
		auto* const transform = transforms.produce();
		if (!tint)
			return; // Block limit reached, no point continuing
		ASSERT(highlight);
		ASSERT(transform);

		// Insert calculated values
		*tint = minoColor(mrsTet.player.type);
		if (mrsTet.player.lockDelay != 0) {
			lockDim.restart();
			lockDim.start -= mrsTet.player.lockDelay * MrsUpdateTick;
			float dim = lockDim.apply();
			tint->r *= dim;
			tint->g *= dim;
			tint->b *= dim;
		}
		*highlight = {1.0f, 1.0f, 1.0f, 0.0f};
		*transform = pieceTransform * minoTransform;
	}
}

/**
 * Queue the ghost piece, if it should be visible.
 */
static void mrsQueueGhost(void)
{
	if (mrsTet.player.state != PlayerActive &&
		mrsTet.player.state != PlayerSpawned)
		return;
	if (mrsTet.player.gravity >= MrsSubGrid)
		return; // Don't show if the game is too fast for it to help
	if (mrsTet.player.lockDelay
		&& (Window::getTime() >= playerPosY.start + playerPosY.duration))
		return; // Don't show if player is on the ground

	ivec2 ghostPos = mrsTet.player.pos;
	while (!pieceOverlapsField(&mrsTet.player.shape, (ivec2){
		ghostPos.x,
		ghostPos.y - 1
	}, mrsTet.field))
		ghostPos.y -= 1; // Drop down as much as possible

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = mrsTet.player.shape[i].x + ghostPos.x;
		float y = mrsTet.player.shape[i].y + ghostPos.y;

		auto* const tint = blockTintsAlpha.produce();
		auto* const highlight = blockHighlightsAlpha.produce();
		auto* const transform = blockTransformsAlpha.produce();
		if (!tint)
			return;
		ASSERT(highlight);
		ASSERT(transform);

		*tint = minoColor(mrsTet.player.type);
		tint->a *= MrsGhostDim;
		*highlight = {1.0f, 1.0f, 1.0f, 0.0f};
		*transform = translate(mat4(1.0f), {x - (signed)(FieldWidth / 2), y, 0.0f});
	}
}

/**
 * Queue the preview piece on top of the field.
 */
static void mrsQueuePreview(void)
{
	if (mrsTet.player.preview == MinoNone)
		return;
	piece previewPiece = {};
	arrayCopy(previewPiece, MrsPieces[mrsTet.player.preview]);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = previewPiece[i].x + MrsPreviewX;
		float y = previewPiece[i].y + MrsPreviewY;
		if (mrsTet.player.preview == MinoI)
			y -= 1;

		const bool transparent = (minoColor(mrsTet.player.preview).a == 1.0);
		auto& tints = transparent? blockTintsOpaque : blockTintsAlpha;
		auto& highlights = transparent? blockHighlightsOpaque : blockHighlightsAlpha;
		auto& transforms = transparent? blockTransformsOpaque : blockTransformsAlpha;
		color4* const tint = tints.produce();
		color4* const highlight = highlights.produce();
		mat4* const transform = transforms.produce();
		if (!tint)
			return; // Block limit reached, no point continuing
		ASSERT(highlight);
		ASSERT(transform);

		*tint = minoColor(mrsTet.player.preview);
		*highlight = {1.0f, 1.0f, 1.0f, 0.0f};
		*transform = translate(mat4(1.0f), {x, y, 0.0f});
	}
}

/**
 * Draw all queued blocks with alpha pre-pass.
 */
static void mrsDrawQueuedBlocks(void)
{
	ASSERT(blockTransformsOpaque.size == blockHighlightsOpaque.size);
	ASSERT(blockTransformsOpaque.size == blockTintsOpaque.size);
	ASSERT(blockTransformsAlpha.size == blockHighlightsAlpha.size);
	ASSERT(blockTransformsAlpha.size == blockTintsAlpha.size);

	modelDraw(block, blockTransformsOpaque.size,
		blockTintsOpaque.data(),
		blockHighlightsOpaque.data(),
		blockTransformsOpaque.data());
	blockTintsOpaque.clear();
	blockHighlightsOpaque.clear();
	blockTransformsOpaque.clear();
	detail::state.setColorWrite(false); // Depth prepass start
	modelDraw(block, blockTransformsAlpha.size,
		blockTintsAlpha.data(),
		blockHighlightsAlpha.data(),
		blockTransformsAlpha.data());
	detail::state.setColorWrite(true); // Depth prepass end
	modelDraw(block, blockTransformsAlpha.size,
		blockTintsAlpha.data(),
		blockHighlightsAlpha.data(),
		blockTransformsAlpha.data());
	blockTintsAlpha.clear();
	blockHighlightsAlpha.clear();
	blockTransformsAlpha.clear();
}

static void mrsQueueBorder(vec3 pos, vec3 size, color4 color)
{
	auto* const tint = borderTints.produce();
	auto* const transform = borderTransforms.produce();
	if (!tint)
		return;
	ASSERT(transform);

	*tint = color;
	const mat4 transformTemp = translate(mat4(1.0f), {pos.x, pos.y, pos.z});
	*transform = glm::scale(transformTemp, {size.x, size.y, size.z});
}

/**
 * Draw the border around the contour of field blocks.
 */
static void mrsDrawBorder(void)
{
	int linesCleared = 0;
	float fallProgress = clearFall.apply();

	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;

		if (mrsTet.linesCleared[y]) {
			linesCleared += 1;
			i += FieldWidth - 1;
			continue;
		}

		if (!fieldGet(mrsTet.field, (ivec2){x, y})) continue;

		// Coords transformed to world space
		float tx = (float)x - (signed)(FieldWidth / 2);
		float ty = (float)y - (float)linesCleared * fallProgress;
		float alpha = MrsBorderDim;
		if (y >= MrsFieldHeightVisible)
			alpha *= MrsExtraRowDim;

		// Left
		if (!fieldGet(mrsTet.field, (ivec2){x - 1, y}))
			mrsQueueBorder((vec3){tx, ty + 0.125f, 0.0f},
				(vec3){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Right
		if (!fieldGet(mrsTet.field, (ivec2){x + 1, y}))
			mrsQueueBorder((vec3){tx + 0.875f, ty + 0.125f, 0.0f},
				(vec3){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down
		if (!fieldGet(mrsTet.field, (ivec2){x, y - 1}))
			mrsQueueBorder((vec3){tx + 0.125f, ty, 0.0f},
				(vec3){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up
		if (!fieldGet(mrsTet.field, (ivec2){x, y + 1}))
			mrsQueueBorder((vec3){tx + 0.125f, ty + 0.875f, 0.0f},
				(vec3){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Left
		if (!fieldGet(mrsTet.field, (ivec2){x - 1, y - 1})
			|| !fieldGet(mrsTet.field, (ivec2){x - 1, y})
			|| !fieldGet(mrsTet.field, (ivec2){x, y - 1}))
			mrsQueueBorder((vec3){tx, ty, 0.0f},
				(vec3){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Right
		if (!fieldGet(mrsTet.field, (ivec2){x + 1, y - 1})
			|| !fieldGet(mrsTet.field, (ivec2){x + 1, y})
			|| !fieldGet(mrsTet.field, (ivec2){x, y - 1}))
			mrsQueueBorder((vec3){tx + 0.875f, ty, 0.0f},
				(vec3){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Left
		if (!fieldGet(mrsTet.field, (ivec2){x - 1, y + 1})
			|| !fieldGet(mrsTet.field, (ivec2){x - 1, y})
			|| !fieldGet(mrsTet.field, (ivec2){x, y + 1}))
			mrsQueueBorder((vec3){tx, ty + 0.875f, 0.0f},
				(vec3){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Right
		if (!fieldGet(mrsTet.field, (ivec2){x + 1, y + 1})
			|| !fieldGet(mrsTet.field, (ivec2){x + 1, y})
			|| !fieldGet(mrsTet.field, (ivec2){x, y + 1}))
			mrsQueueBorder((vec3){tx + 0.875f, ty + 0.875f, 0.0f},
				(vec3){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
	}

	ASSERT(borderTints.size == borderTransforms.size);
	modelDraw(border, borderTransforms.size,
		borderTints.data(), nullptr, borderTransforms.data());
	borderTints.clear();
	borderTransforms.clear();
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

#include "meshes/scene.mesh"
#include "meshes/guide.mesh"
#include "meshes/block.mesh"
#include "meshes/border.mesh"
void mrsDrawInit(void)
{
	if (initialized) return;

	scene = modelCreateFlat("scene", sceneMeshSize, sceneMesh);
	guide = modelCreateFlat("scene", guideMeshSize, guideMesh);
	block = modelCreatePhong("block", blockMeshSize, blockMesh, blockMeshMat);
	border = modelCreateFlat("border", borderMeshSize, borderMesh);

	initialized = true;
	L.debug("Mrs draw initialized");
}

void mrsDrawCleanup(void)
{
	if (!initialized) return;

	modelDestroy(border);
	border = nullptr;
	modelDestroy(block);
	block = nullptr;
	modelDestroy(guide);
	guide = nullptr;
	modelDestroy(scene);
	scene = nullptr;

	initialized = false;
	L.debug("Mrs draw cleaned up");
}

void mrsDraw(void)
{
	ASSERT(initialized);

	glClearColor(0.0185f, 0.029f, 0.0944f, 1.0f); //TODO make into layer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	worldAmbientColor = (color3){0.0185f, 0.029f, 0.0944f};
	mrsDrawScene();
	mrsDrawGuide();
	mrsQueueField();
	mrsQueuePlayer();
	mrsQueueGhost();
	mrsQueuePreview();
	mrsDrawQueuedBlocks();
	mrsDrawBorder();
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
					(float)x - (float)FieldWidth / 2,
					(float)row + 0.0625f + 0.125f * (float)ySub,
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
				(float)x - (float)FieldWidth / 2,
				(float)row,
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
				(float)x - (float)FieldWidth / 2,
				(float)y,
				0.0f
			}, 8, params);
		}
	}
}
