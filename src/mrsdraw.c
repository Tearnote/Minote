/**
 * Implementation of mrsdraw.h
 * @file
 */

#include "mrsdraw.h"

#include <assert.h>
#include "particles.h"
#include "mrsdef.h"
#include "debug.h"
#include "model.h"
#include "world.h"
#include "mrs.h"
#include "log.h"

static Model* scene = null;
static Model* guide = null;

static Model* block = null;
static darray* blockTintsOpaque = null;
static darray* blockHighlightsOpaque = null;
static darray* blockTransformsOpaque = null;
static darray* blockTintsAlpha = null;
static darray* blockHighlightsAlpha = null;
static darray* blockTransformsAlpha = null;

static Model* border = null;
static darray* borderTints = null;
static darray* borderTransforms = null;

static bool initialized = false;

/// Last player position as seen by the drawing system
static point2i lastPlayerPos = {0, 0};

/// Last number of player piece 90-degree turns as seen by the drawing system
static int lastPlayerRotation = 0;

/// Tweening of player piece position
static Tween playerPosX = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = EaseOutExponential
};

static Tween playerPosY = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = EaseOutExponential
};

/// Tweening of player piece rotation
static Tween playerRotation = {
	.from = 0.0f,
	.to = 0.0f,
	.duration = 3 * MrsUpdateTick,
	.type = EaseOutExponential
};

/// Player piece animation after the piece locks
static Tween lockFlash = {
	.from = 1.0f,
	.to = 0.0f,
	.duration = 8 * MrsUpdateTick,
	.type = EaseLinear
};

/// Player piece animation as the lock delay ticks down
static Tween lockDim = {
	.from = 1.0f,
	.to = 0.1f,
	.duration = MrsLockDelay * MrsUpdateTick,
	.type = EaseInQuadratic
};

/// Animation of the scene when combo counter changes
static Tween comboFade = {
	.from = 1.1f,
	.to = 1.1f,
	.duration = 24 * MrsUpdateTick,
	.type = EaseOutQuadratic
};

/// Thump animation of a falling stack
static Tween clearFall = {
	.from = 0.0f,
	.to = 1.0f,
	.duration = MrsClearDelay * MrsUpdateTick,
	.type = EaseInCubic
};

/// Sparks released on line clear
static ParticleParams particlesClear = {
	.color = {0.0f, 0.0f, 0.0f, 1.0f}, // runtime
	.durationMin = secToNsec(0),
	.durationMax = secToNsec(1.5),
	.distanceMin = 3.2f, // runtime
	.distanceMax = 6.4f, // runtime
	.spinMin = 0.001f,
	.spinMax = 0.3f,
	.directionVert = 0,
	.directionHorz = 0,
	.ease = EaseOutQuartic
};

/// Cloud of dust caused by a player piece falling on the stack
/// or finishing line clear thump
static ParticleParams particlesThump = {
	.color = {0.6f, 0.6f, 0.6f, 0.8f},
	.durationMin = secToNsec(0.4),
	.durationMax = secToNsec(0.8),
	.distanceMin = 0.2f,
	.distanceMax = 1.2f,
	.spinMin = 0.4f,
	.spinMax = 1.6f,
	.directionVert = 1,
	.directionHorz = 0,
	.ease = EaseOutExponential
};

/// Sparks of a player piece being shifted across the playfield
static ParticleParams particlesSlide = {
	.color = {0.0f, 0.4f, 2.0f, 1.0f},
	.durationMin = secToNsec(0.3),
	.durationMax = secToNsec(0.6),
	.distanceMin = 0.2f,
	.distanceMax = 1.4f,
	.spinMin = 0.4f,
	.spinMax = 1.2f,
	.directionVert = 1,
	.directionHorz = 0, // runtime
	.ease = EaseOutExponential
};

/// Sparks of a player piece being DASed across the playfield
static ParticleParams particlesSlideFast = {
	.color = {2.0f, 0.4f, 0.0f, 1.0f},
	.durationMin = secToNsec(0.25),
	.durationMax = secToNsec(0.5),
	.distanceMin = 0.4f,
	.distanceMax = 2.0f,
	.spinMin = 0.4f,
	.spinMax = 1.2f,
	.directionVert = 1,
	.directionHorz = 0, // runtime
	.ease = EaseOutExponential
};

/**
 * Create a dust cloud effect under the player piece.
 */
static void mrsEffectDrop(void)
{
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = mrsTet.player.pos.x + mrsTet.player.shape[i].x;
		int y = mrsTet.player.pos.y + mrsTet.player.shape[i].y;
		if (fieldGet(mrsTet.field, (point2i){x, y - 1})) {
			particlesGenerate((point3f){
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
	float boost = tweenApply(&comboFade);
	modelDraw(scene, 1, (color4[]){{boost, boost, boost, 1.0f}}, null,
		&IdentityMatrix);
}

/**
 * Draw the guide model, helping a beginner player keep track of columns.
 */
static void mrsDrawGuide(void)
{
	modelDraw(guide, 1, (color4[]){Color4White}, null, &IdentityMatrix);
}

/**
 * Queue the contents of the tetrion field.
 */
static void mrsQueueField(void)
{
	int linesCleared = 0;
	float fallProgress = tweenApply(&clearFall);

	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;

		if (mrsTet.linesCleared[y]) {
			linesCleared += 1;
			i += FieldWidth - 1;
			continue;
		}

		mino type = fieldGet(mrsTet.field, (point2i){x, y});
		if (type == MinoNone) continue;

		color4* tint = null;
		color4* highlight = null;
		mat4x4* transform = null;
		if (minoColor(type).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			highlight = darrayProduce(blockHighlightsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			highlight = darrayProduce(blockHighlightsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(type));
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
			float flash = tweenApply(&lockFlash);
			color4Copy(*highlight,
				((color4){MrsLockFlashBrightness, MrsLockFlashBrightness,
				          MrsLockFlashBrightness, flash}));
		} else {
			color4Copy(*highlight, Color4Clear);
		}

		float fx = (float)x;
		float fy = (float)y - (float)linesCleared * fallProgress;
		mat4x4_translate(*transform, fx - (signed)(FieldWidth / 2), fy, 0.0f);
	}
}

/**
 * Queue the player piece on top of the field.
 */
static void mrsQueuePlayer(void)
{
	// Tween the player position
	if (mrsTet.player.pos.x != lastPlayerPos.x) {
		playerPosX.from = tweenApply(&playerPosX);
		playerPosX.to = mrsTet.player.pos.x;
		if (mrsTet.player.autoshiftCharge == MrsAutoshiftCharge) {
			playerPosX.duration = 1 * MrsUpdateTick;
			playerPosX.type = EaseLinear;
		} else {
			playerPosX.duration = 3 * MrsUpdateTick;
			playerPosX.type = EaseOutExponential;
		}
		tweenRestart(&playerPosX);
		lastPlayerPos.x = mrsTet.player.pos.x;
	}
	if (mrsTet.player.pos.y != lastPlayerPos.y) {
		playerPosY.from = tweenApply(&playerPosY);
		playerPosY.to = mrsTet.player.pos.y;
		tweenRestart(&playerPosY);
		lastPlayerPos.y = mrsTet.player.pos.y;
	}

	// Tween the player rotation
	if (mrsTet.player.rotation != mod(lastPlayerRotation, SpinSize)) {
		int delta = mrsTet.player.rotation - mod(lastPlayerRotation, SpinSize);
		if (delta == 3) delta -= 4;
		if (delta == -3) delta += 4;
		playerRotation.from = tweenApply(&playerRotation);
		lastPlayerRotation += delta;
		playerRotation.to = lastPlayerRotation;
		tweenRestart(&playerRotation);
	}

	// Stop if no drawing needed
	if (mrsTet.player.state != PlayerActive &&
		mrsTet.player.state != PlayerSpawned)
		return;

	// Get player piece shape (not rotated)
	piece player = {0};
	arrayCopy(player, MrsPieces[mrsTet.player.type]);

	// Get piece transform (piece position and rotation)
	mat4x4 pieceTranslation = {0};
	mat4x4 pieceRotation = {0};
	mat4x4 pieceRotationTemp = {0};
	mat4x4 pieceTransform = {0};
	mat4x4_translate(pieceTranslation,
		tweenApply(&playerPosX) - (signed)(FieldWidth / 2),
		tweenApply(&playerPosY), 0.0f);
	mat4x4_translate(pieceRotationTemp, 0.5f, 0.5f, 0.0f);
	mat4x4_rotate_Z(pieceRotation, pieceRotationTemp,
		tweenApply(&playerRotation) * radf(90));
	mat4x4_translate_in_place(pieceRotation, -0.5f, -0.5f, 0.0f);
	mat4x4_mul(pieceTransform, pieceTranslation, pieceRotation);

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		// Get mino transform (offset from piece origin)
		mat4x4 minoTransform = {0};
		mat4x4_translate(minoTransform, player[i].x, player[i].y, 0.0f);

		// Queue up next mino
		color4* tint = null;
		color4* highlight = null;
		mat4x4* transform = null;
		if (minoColor(mrsTet.player.type).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			highlight = darrayProduce(blockHighlightsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			highlight = darrayProduce(blockHighlightsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		// Insert calculated values
		color4Copy(*tint, minoColor(mrsTet.player.type));
		if (mrsTet.player.lockDelay != 0) {
			tweenRestart(&lockDim);
			lockDim.start -= mrsTet.player.lockDelay * MrsUpdateTick;
			float dim = tweenApply(&lockDim);
			tint->r *= dim;
			tint->g *= dim;
			tint->b *= dim;
		}
		color4Copy(*highlight, Color4Clear);
		mat4x4_mul(*transform, pieceTransform, minoTransform);
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
	if (mrsTet.player.gravity >= MrsSubGrid || mrsTet.player.lockDelay)
		return;

	point2i ghostPos = mrsTet.player.pos;
	while (!pieceOverlapsField(&mrsTet.player.shape, (point2i){
		ghostPos.x,
		ghostPos.y - 1
	}, mrsTet.field))
		ghostPos.y -= 1; // Drop down as much as possible

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = mrsTet.player.shape[i].x + ghostPos.x;
		float y = mrsTet.player.shape[i].y + ghostPos.y;

		color4* tint = darrayProduce(blockTintsAlpha);
		color4* highlight = darrayProduce(blockHighlightsAlpha);
		mat4x4* transform = darrayProduce(blockTransformsAlpha);

		color4Copy(*tint, minoColor(mrsTet.player.type));
		tint->a *= MrsGhostDim;
		color4Copy(*highlight, Color4Clear);
		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
	}
}

/**
 * Queue the preview piece on top of the field.
 */
static void mrsQueuePreview(void)
{
	if (mrsTet.player.preview == MinoNone)
		return;
	piece previewPiece = {0};
	arrayCopy(previewPiece, MrsPieces[mrsTet.player.preview]);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = previewPiece[i].x + MrsPreviewX;
		float y = previewPiece[i].y + MrsPreviewY;
		if (mrsTet.player.preview == MinoI)
			y -= 1;

		color4* tint = null;
		color4* highlight = null;
		mat4x4* transform = null;
		if (minoColor(mrsTet.player.preview).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			highlight = darrayProduce(blockHighlightsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			highlight = darrayProduce(blockHighlightsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(mrsTet.player.preview));
		color4Copy(*highlight, Color4Clear);
		mat4x4_translate(*transform, x, y, 0.0f);
	}
}

/**
 * Draw all queued blocks with alpha pre-pass.
 */
static void mrsDrawQueuedBlocks(void)
{
	modelDraw(block, blockTransformsOpaque->count,
		(color4*)blockTintsOpaque->data,
		(color4*)blockHighlightsOpaque->data,
		(mat4x4*)blockTransformsOpaque->data);
	darrayClear(blockTintsOpaque);
	darrayClear(blockHighlightsOpaque);
	darrayClear(blockTransformsOpaque);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Depth prepass start
	modelDraw(block, blockTransformsAlpha->count,
		(color4*)blockTintsAlpha->data,
		(color4*)blockHighlightsAlpha->data,
		(mat4x4*)blockTransformsAlpha->data);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Depth prepass end
	modelDraw(block, blockTransformsAlpha->count,
		(color4*)blockTintsAlpha->data,
		(color4*)blockHighlightsAlpha->data,
		(mat4x4*)blockTransformsAlpha->data);
	darrayClear(blockTintsAlpha);
	darrayClear(blockHighlightsAlpha);
	darrayClear(blockTransformsAlpha);
}

static void mrsQueueBorder(point3f pos, size3f size, color4 color)
{
	color4* tint = darrayProduce(borderTints);
	mat4x4* transform = darrayProduce(borderTransforms);
	color4Copy(*tint, color);
	mat4x4_identity(*transform);
	mat4x4_translate_in_place(*transform, pos.x, pos.y, pos.z);
	mat4x4_scale_aniso(*transform, *transform, size.x, size.y, size.z);
}

/**
 * Draw the border around the contour of field blocks.
 */
static void mrsDrawBorder(void)
{
	int linesCleared = 0;
	float fallProgress = tweenApply(&clearFall);

	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;

		if (mrsTet.linesCleared[y]) {
			linesCleared += 1;
			i += FieldWidth - 1;
			continue;
		}

		if (!fieldGet(mrsTet.field, (point2i){x, y})) continue;

		// Coords transformed to world space
		float tx = (float)x - (signed)(FieldWidth / 2);
		float ty = (float)y - (float)linesCleared * fallProgress;
		float alpha = MrsBorderDim;
		if (y >= MrsFieldHeightVisible)
			alpha *= MrsExtraRowDim;

		// Left
		if (!fieldGet(mrsTet.field, (point2i){x - 1, y}))
			mrsQueueBorder((point3f){tx, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Right
		if (!fieldGet(mrsTet.field, (point2i){x + 1, y}))
			mrsQueueBorder((point3f){tx + 0.875f, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down
		if (!fieldGet(mrsTet.field, (point2i){x, y - 1}))
			mrsQueueBorder((point3f){tx + 0.125f, ty, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up
		if (!fieldGet(mrsTet.field, (point2i){x, y + 1}))
			mrsQueueBorder((point3f){tx + 0.125f, ty + 0.875f, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Left
		if (!fieldGet(mrsTet.field, (point2i){x - 1, y - 1})
			|| !fieldGet(mrsTet.field, (point2i){x - 1, y})
			|| !fieldGet(mrsTet.field, (point2i){x, y - 1}))
			mrsQueueBorder((point3f){tx, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Right
		if (!fieldGet(mrsTet.field, (point2i){x + 1, y - 1})
			|| !fieldGet(mrsTet.field, (point2i){x + 1, y})
			|| !fieldGet(mrsTet.field, (point2i){x, y - 1}))
			mrsQueueBorder((point3f){tx + 0.875f, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Left
		if (!fieldGet(mrsTet.field, (point2i){x - 1, y + 1})
			|| !fieldGet(mrsTet.field, (point2i){x - 1, y})
			|| !fieldGet(mrsTet.field, (point2i){x, y + 1}))
			mrsQueueBorder((point3f){tx, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Right
		if (!fieldGet(mrsTet.field, (point2i){x + 1, y + 1})
			|| !fieldGet(mrsTet.field, (point2i){x + 1, y})
			|| !fieldGet(mrsTet.field, (point2i){x, y + 1}))
			mrsQueueBorder((point3f){tx + 0.875f, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
	}

	modelDraw(border, borderTransforms->count,
		(color4*)borderTints->data, null, (mat4x4*)borderTransforms->data);
	darrayClear(borderTints);
	darrayClear(borderTransforms);
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
				mino cell = fieldGet(mrsTet.field, (point2i){x, y});
				color4 cellColor = minoColor(cell);
				if (nk_button_color(nkCtx(), nk_rgba(
					cellColor.r * 255.0f,
					cellColor.g * 255.0f,
					cellColor.b * 255.0f,
					cellColor.a * 255.0f))) {
					if (cell)
						fieldSet(mrsTet.field, (point2i){x, y}, MinoNone);
					else
						fieldSet(mrsTet.field, (point2i){x, y}, MinoGarbage);
				}
			}
		}
	}
	nk_end(nkCtx());
}

void mrsDrawInit(void)
{
	if (initialized) return;

	scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	guide = modelCreateFlat(u8"scene",
#include "meshes/guide.mesh"
	);
	block = modelCreatePhong(u8"block",
#include "meshes/block.mesh"
	);
	blockTintsOpaque = darrayCreate(sizeof(color4));
	blockHighlightsOpaque = darrayCreate(sizeof(color4));
	blockTransformsOpaque = darrayCreate(sizeof(mat4x4));
	blockTintsAlpha = darrayCreate(sizeof(color4));
	blockHighlightsAlpha = darrayCreate(sizeof(color4));
	blockTransformsAlpha = darrayCreate(sizeof(mat4x4));
	border = modelCreateFlat(u8"border",
#include "meshes/border.mesh"
	);
	borderTints = darrayCreate(sizeof(color4));
	borderTransforms = darrayCreate(sizeof(mat4x4));

	initialized = true;
	logDebug(applog, u8"Mrs draw initialized");
}

void mrsDrawCleanup(void)
{
	if (!initialized) return;

	darrayDestroy(borderTransforms);
	borderTransforms = null;
	darrayDestroy(borderTints);
	borderTints = null;
	modelDestroy(border);
	border = null;
	darrayDestroy(blockTransformsAlpha);
	blockTransformsAlpha = null;
	darrayDestroy(blockHighlightsAlpha);
	blockHighlightsAlpha = null;
	darrayDestroy(blockTintsAlpha);
	blockTintsAlpha = null;
	darrayDestroy(blockTransformsOpaque);
	blockTransformsOpaque = null;
	darrayDestroy(blockHighlightsOpaque);
	blockHighlightsOpaque = null;
	darrayDestroy(blockTintsOpaque);
	blockTintsOpaque = null;
	modelDestroy(block);
	block = null;
	modelDestroy(guide);
	guide = null;
	modelDestroy(scene);
	scene = null;

	initialized = false;
	logDebug(applog, u8"Mrs draw cleaned up");
}

void mrsDraw(void)
{
	assert(initialized);

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
	structCopy(lastPlayerPos, mrsTet.player.pos);
	lastPlayerRotation = mrsTet.player.rotation;
	playerPosX.from = lastPlayerPos.x;
	playerPosX.to = lastPlayerPos.x;
	playerPosY.from = lastPlayerPos.y + 1;
	playerPosY.to = lastPlayerPos.y;
	playerRotation.from = lastPlayerRotation;
	playerRotation.to = lastPlayerRotation;
	tweenRestart(&playerPosX);
	tweenRestart(&playerPosY);
	tweenRestart(&playerRotation);
}

void mrsEffectLock(void)
{
	tweenRestart(&lockFlash);
}

void mrsEffectClear(int row, int power)
{
	for (int x = 0; x < FieldWidth; x += 1) {
		for (int ySub = 0; ySub < 8; ySub += 1) {
			color4 cellColor = minoColor(
				fieldGet(mrsTet.field, (point2i){x, row}));
			color4Copy(particlesClear.color, cellColor);
			particlesClear.color.r *= MrsParticlesClearBoost;
			particlesClear.color.g *= MrsParticlesClearBoost;
			particlesClear.color.b *= MrsParticlesClearBoost;
			particlesClear.distanceMin = 3.2f * power;
			particlesClear.distanceMax = particlesClear.distanceMin * 2.0f;
			particlesGenerate((point3f){
					(float)x - (float)FieldWidth / 2,
					(float)row + 0.0625f + 0.125f * (float)ySub,
					0.0f},
				power, &particlesClear);
		}
	}

	tweenRestart(&clearFall);
}

void mrsEffectThump(int row)
{
	for (int x = 0; x < FieldWidth; x += 1) {
		if (fieldGet(mrsTet.field, (point2i){x, row})
			&& fieldGet(mrsTet.field, (point2i){x, row - 1}))
			particlesGenerate((point3f){
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
		if (fieldGet(mrsTet.field, (point2i){x, y - 1})) {
			particlesGenerate((point3f){
				(float)x - (float)FieldWidth / 2,
				(float)y,
				0.0f
			}, 8, params);
		}
	}
}
