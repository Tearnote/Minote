/**
 * Implementation of mrs.h
 * @file
 */

#include "mrs.h"

#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "mrstables.h"
#include "particles.h"
#include "renderer.h"
#include "debug.h"
#include "model.h"
#include "world.h"
#include "log.h"

#define SpawnX 3 ///< X position of player piece spawn
#define SpawnY 18 ///< Y position of player piece spawn
#define SubGrid 256 ///< Number of subpixels per cell, used for gravity

#define StartingTokens 6 ///< Number of tokens that each piece starts with

#define AutoshiftCharge 16 ///< Frames direction has to be held before autoshift
#define AutoshiftRepeat 1 ///< Frames between autoshifts
#define LockDelay 40 ///< Frames a piece can spend on the stack before locking
#define ClearOffset 5 ///< Frames between piece lock and line clear
#define ClearDelay 30 ///< Frames between line clear and thump
#define SpawnDelay 30 ///< Frames between lock/thump and new piece spawn

// Debug switches
static int debugPauseSpawn = 0; // Boolean, int for compatibility
static int debugInfLock = 0; // Boolean, int for compatibility

static bool initialized = false;

Tetrion mrsTet = {0};

/**
 * Test whether an input has been pressed just now.
 * @param type ::InputType to test
 * @return true if just pressed, false otherwise
 */
#define inputPressed(type) \
    (mrsTet.player.inputMap[(type)] && !mrsTet.player.inputMapPrev[(type)])

/**
 * Test whether an input is pressed during this frame.
 * @param type ::InputType to test
 * @return true if pressed, false if not pressed
 */
#define inputHeld(type) \
    (mrsTet.player.inputMap[(type)])

////////////////////////////////////////////////////////////////////////////////

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

#define FieldHeightVisible 20u ///< Number of bottom rows the player can see
#define PreviewX -2.0f ///< X offset of preview piece
#define PreviewY 21.0f ///< Y offset of preview piece
#define FieldDim 0.3f ///< Multiplier of field block color
#define ExtraRowDim 0.25f ///< Multiplier of field block alpha above the scene
#define GhostDim 0.2f ///< Multiplier of ghost block alpha
#define BorderDim 0.5f ///< Multiplier of border alpha
#define LockFlashBrightness 1.2f ///< Color value of lock flash highlight
#define ParticlesClearBoost 1.4f ///< Intensity multiplier for line clear effect

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
	.duration = LockDelay * MrsUpdateTick,
	.type = EaseInQuadratic
};

/// Animation of the scene when combo counter changes
static Tween comboFade = {
	.from = 1.1f,
	.to = 1.1f,
	.duration = 24 * MrsUpdateTick,
	.type = EaseOutQuadratic
};

static Tween clearFall = {
	.from = 0.0f,
	.to = 1.0f,
	.duration = ClearDelay * MrsUpdateTick,
	.type = EaseInCubic
};

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

static void genParticlesClear(int row, int power);
static void genParticlesThump(int row);
static void genParticlesDrop(void);
static void genParticlesSlide(int direction, bool fast);

/**
 * Try to kick the player piece into a legal position.
 * @param prevRotation piece rotation prior to the kick
 * @return true if player piece already legal or successfully kicked, false if
 * no kick was possible
 */
static bool tryKicks(spin prevRotation)
{
	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		return true; // Original position

	if (mrsTet.player.state == PlayerSpawned)
		return false; // If this is IRS, don't attempt kicks
	if (mrsTet.player.type == MinoI)
		return false; // I doesn't kick

	// L/J/T floorkick
	if ((mrsTet.player.type == MinoL ||
		mrsTet.player.type == MinoJ ||
		mrsTet.player.type == MinoT) &&
		prevRotation == Spin180) {
		mrsTet.player.pos.y += 1;
		if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
			return true; // 1 to the right
		mrsTet.player.pos.y -= 1;
	}

	// Now that every exception is filtered out, we can try the default kicks
	int preference = mrsTet.player.lastDirection == InputRight ? 1 : -1;

	// Down
	mrsTet.player.pos.y -= 1;
	if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the right
	mrsTet.player.pos.y += 1;

	// Left/right
	mrsTet.player.pos.x += preference;
	if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the right
	mrsTet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the left
	mrsTet.player.pos.x += preference;

	// Down+left/right
	mrsTet.player.pos.y -= 1;
	mrsTet.player.pos.x += preference;
	if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the right
	mrsTet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the left
	mrsTet.player.pos.x += preference;
	mrsTet.player.pos.y += 1;

	return false; // Failure, returned to original position
}

/**
 * Attempt to rotate the player piece in the specified direction, kicking the
 * piece if needed.
 * @param direction -1 for clockwise, 1 for counter-clockwise
 */
static void rotate(int direction)
{
	assert(direction == 1 || direction == -1);
	spin prevRotation = mrsTet.player.rotation;
	point2i prevPosition = mrsTet.player.pos;

	if (direction == 1)
		spinClockwise(&mrsTet.player.rotation);
	else
		spinCounterClockwise(&mrsTet.player.rotation);

	// Apply crawl offsets to I,S,Z
	if (mrsTet.player.type == MinoI ||
		mrsTet.player.type == MinoS ||
		mrsTet.player.type == MinoZ) {
		if (direction == -1 &&
			(prevRotation == Spin90 ||
				prevRotation == Spin270))
			mrsTet.player.pos.x += 1;
		if (direction == 1 &&
			(prevRotation == SpinNone ||
				prevRotation == Spin180)) {
			mrsTet.player.pos.x -= 1;
		}
	}

	if (!tryKicks(prevRotation)) {
		mrsTet.player.rotation = prevRotation;
		mrsTet.player.pos.x = prevPosition.x;
		mrsTet.player.pos.y = prevPosition.y;
	}
}

/**
 * Attempt to shift the player piece in the given direction.
 * @param direction -1 for left, 1 for right
 */
static void shift(int direction)
{
	assert(direction == 1 || direction == -1);
	mrsTet.player.pos.x += direction;
	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	if (pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field)) {
		mrsTet.player.pos.x -= direction;
	} else {
		mrsTet.player.pos.x -= direction;
		genParticlesSlide(direction,
			(mrsTet.player.autoshiftCharge == AutoshiftCharge));
		mrsTet.player.pos.x += direction;
	}
}

/**
 * Return a random new piece type, making use of the token system.
 * @return Picked piece type
 */
static mino randomPiece(void)
{
	// Count the number of tokens
	size_t tokenTotal = 0;
	for (size_t i = 0; i < countof(mrsTet.player.tokens); i += 1)
		if (mrsTet.player.tokens[i] > 0) tokenTotal += mrsTet.player.tokens[i];
	assert(tokenTotal);

	// Create and fill the token list
	int tokenList[tokenTotal];
	size_t tokenListIndex = 0;
	for (size_t i = 0; i < countof(mrsTet.player.tokens); i += 1) {
		if (mrsTet.player.tokens[i] <= 0) continue;
		for (size_t j = 0; j < mrsTet.player.tokens[i]; j += 1) {
			assert(tokenListIndex < tokenTotal);
			tokenList[tokenListIndex] = i;
			tokenListIndex += 1;
		}
	}
	assert(tokenListIndex == tokenTotal);

	// Pick a random token from the list and update the token distribution
	int picked = tokenList[rngInt(mrsTet.rng, tokenTotal)];
	for (size_t i = 0; i < countof(mrsTet.player.tokens); i += 1) {
		if (i == picked)
			mrsTet.player.tokens[i] -= countof(mrsTet.player.tokens) - 1;
		else
			mrsTet.player.tokens[i] += 1;
	}

	return picked + MinoNone + 1;
}

/**
 * Stop the round.
 */
static void gameOver(void)
{
	mrsTet.state = TetrionOutro;
}

/**
 * Prepare the player piece for a brand new adventure at the top of the field.
 */
static void spawnPiece(void)
{
	mrsTet.player.state = PlayerSpawned; // Some moves restricted on first frame
	mrsTet.player.pos.x = SpawnX;
	mrsTet.player.pos.y = SpawnY;
	mrsTet.player.yLowest = mrsTet.player.pos.y;

	// Picking the next piece
	mrsTet.player.type = mrsTet.player.preview;
	mrsTet.player.preview = randomPiece();

	if (mrsTet.player.type == MinoI)
		mrsTet.player.pos.y -= 1; // I starts lower than other pieces
	mrsTet.player.ySub = 0;
	mrsTet.player.lockDelay = 0;
	mrsTet.player.spawnDelay = 0;
	mrsTet.player.clearDelay = 0;
	mrsTet.player.rotation = SpinNone;

	// IRS
	if (inputHeld(InputButton2)) {
		rotate(-1);
	} else {
		if (inputHeld(InputButton1) || inputHeld(InputButton3))
			rotate(1);
	}

	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	if (pieceOverlapsField(playerPiece, mrsTet.player.pos, mrsTet.field))
		gameOver();

	// Increase gravity
	if (mrsTet.player.gravity >= 20 * SubGrid) return;
	int level = mrsTet.player.gravity / 64 + 1;
	mrsTet.player.gravity += level;
}

/**
 * Check if field row for full lines and initiate clears.
 * @return Number of lines cleared
 */
static int checkClears(void)
{
	int count = 0;
	for (int y = 0; y < FieldHeight; y += 1) {
		if (!fieldIsRowFull(mrsTet.field, y))
			continue;
		count += 1;
		mrsTet.linesCleared[y] = true;
	}

	for (int y = 0; y < FieldHeight; y += 1) {
		if (!mrsTet.linesCleared[y]) continue;
		genParticlesClear(y, count);
		fieldClearRow(mrsTet.field, y);
	}

	if (count) tweenRestart(&clearFall);
	return count;
}

/**
 * "Thump" previously cleared lines, bringing them crashing into the ground.
 */
static void thump(void)
{
	int offset = 0;
	for (int y = 0; y < FieldHeight; y += 1) {
		if (!mrsTet.linesCleared[y + offset])
			continue; // Drop only above cleared lines
		fieldDropRow(mrsTet.field, y);
		mrsTet.linesCleared[y + offset] = false;
		genParticlesThump(y);
		y -= 1;
		offset += 1;
	}
}

/**
 * Check whether the player piece could move down one cell without overlapping
 * the field.
 * @return true if no overlap, false if overlap
 */
static bool canDrop(void)
{
	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	return !pieceOverlapsField(playerPiece, (point2i){
		.x = mrsTet.player.pos.x,
		.y = mrsTet.player.pos.y - 1
	}, mrsTet.field);
}

/**
 * Move the player piece down one cell if possible, also calculating other
 * appropriate values.
 */
static void drop(void)
{
	if (!canDrop())
		return;

	mrsTet.player.pos.y -= 1;

	// Reset lock delay if piece dropped lower than ever
	if (mrsTet.player.pos.y < mrsTet.player.yLowest) {
		mrsTet.player.lockDelay /= 2;
		mrsTet.player.yLowest = mrsTet.player.pos.y;
	}

	if (inputHeld(InputLeft))
		genParticlesSlide(-1, (mrsTet.player.autoshiftCharge == AutoshiftCharge));
	else if (inputHeld(InputRight))
		genParticlesSlide(1, (mrsTet.player.autoshiftCharge == AutoshiftCharge));
	else
		genParticlesDrop();
}

/**
 * Stamp player piece onto the grid.
 */
static void lock(void)
{
	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	fieldStampPiece(mrsTet.field, playerPiece, mrsTet.player.pos, mrsTet.player.type);
	mrsTet.player.state = PlayerSpawn;
	tweenRestart(&lockFlash);
}

void mrsInit(void)
{
	if (initialized) return;

	// Logic init
	structClear(mrsTet);
	mrsTet.frame = -1;
	mrsTet.ready = 3 * 50;
	mrsTet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	mrsTet.player.autoshiftDelay = AutoshiftRepeat; // Starts out pre-charged
	mrsTet.player.spawnDelay = SpawnDelay; // Start instantly
	mrsTet.player.gravity = 3;

	mrsTet.rng = rngCreate((uint64_t)time(null));
	for (size_t i = 0; i < countof(mrsTet.player.tokens); i += 1)
		mrsTet.player.tokens[i] = StartingTokens;
	mrsTet.player.preview = randomPiece();

	mrsTet.state = TetrionReady;

	// Render init
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
	logDebug(applog, u8"Mrs sublayer initialized");
}

void mrsCleanup(void)
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
	rngDestroy(mrsTet.rng);
	mrsTet.rng = null;
	fieldDestroy(mrsTet.field);
	mrsTet.field = null;
	initialized = false;
	logDebug(applog, u8"Mrs sublayer cleaned up");
}

/**
 * Populate and rotate the input arrays for press and hold detection.
 * @param inputs List of this frame's new inputs
 */
static void mrsUpdateInputs(darray* inputs)
{
	assert(inputs);

	// Update raw inputs
	if (mrsTet.state != TetrionOutro) {
		for (size_t i = 0; i < inputs->count; i += 1) {
			Input* in = darrayGet(inputs, i);
			assert(in->type < InputSize);
			mrsTet.player.inputMapRaw[in->type] = in->state;
		}
	} else { // Force-release everything on gameover
		arrayClear(mrsTet.player.inputMapRaw);
	}

	// Rotate the input arrays
	arrayCopy(mrsTet.player.inputMapPrev, mrsTet.player.inputMap);
	arrayCopy(mrsTet.player.inputMap, mrsTet.player.inputMapRaw);

	// Filter conflicting inputs
	if (mrsTet.player.inputMap[InputDown] || mrsTet.player.inputMap[InputUp]) {
		mrsTet.player.inputMap[InputLeft] = false;
		mrsTet.player.inputMap[InputRight] = false;
	}
	if (mrsTet.player.inputMap[InputLeft] && mrsTet.player.inputMap[InputRight]) {
		if (mrsTet.player.lastDirection == InputLeft)
			mrsTet.player.inputMap[InputRight] = false;
		if (mrsTet.player.lastDirection == InputRight)
			mrsTet.player.inputMap[InputLeft] = false;
	}

	// Update last direction
	if (inputHeld(InputLeft))
		mrsTet.player.lastDirection = InputLeft;
	else if (inputHeld(InputRight))
		mrsTet.player.lastDirection = InputRight;
}

/**
 * Check for state triggers and progress through states.
 */
static void mrsUpdateState(void)
{
	if (mrsTet.state == TetrionReady) {
		mrsTet.ready -= 1;
		if (mrsTet.ready == 0)
			mrsTet.state = TetrionPlaying;
	} else if (mrsTet.state == TetrionPlaying) {
		mrsTet.frame += 1;
	}
	if (mrsTet.player.state == PlayerSpawned)
		mrsTet.player.state = PlayerActive;
}

/**
 * Spin the player piece.
 */
static void mrsUpdateRotation(void)
{
	if (mrsTet.player.state != PlayerActive)
		return;
	if (inputPressed(InputButton2))
		rotate(-1);
	if (inputPressed(InputButton1) || inputPressed(InputButton3))
		rotate(1);
}

/**
 * Shift the player piece, either through a direct press or autoshift.
 */
static void mrsUpdateShift(void)
{
	// Check requested movement direction
	int shiftDirection = 0;
	if (inputHeld(InputLeft))
		shiftDirection = -1;
	else if (inputHeld(InputRight))
		shiftDirection = 1;

	// If not moving or moving in the opposite direction of ongoing DAS,
	// reset DAS and shift instantly
	if (!shiftDirection || shiftDirection != mrsTet.player.autoshiftDirection) {
		mrsTet.player.autoshiftDirection = shiftDirection;
		mrsTet.player.autoshiftCharge = 0;
		mrsTet.player.autoshiftDelay = AutoshiftRepeat; // Starts out pre-charged
		if (shiftDirection && mrsTet.player.state == PlayerActive)
			shift(shiftDirection);
	}

	// If moving, advance and apply DAS
	if (!shiftDirection)
		return;
	if (mrsTet.player.autoshiftCharge < AutoshiftCharge)
		mrsTet.player.autoshiftCharge += 1;
	if (mrsTet.player.autoshiftCharge == AutoshiftCharge) {
		if (mrsTet.player.autoshiftDelay < AutoshiftRepeat)
			mrsTet.player.autoshiftDelay += 1;

		// If during ARE, keep the DAS charged
		if (mrsTet.player.autoshiftDelay >= AutoshiftRepeat
			&& mrsTet.player.state == PlayerActive) {
			mrsTet.player.autoshiftDelay = 0;
			shift(mrsTet.player.autoshiftDirection);
		}
	}
}

/**
 * Check for cleared lines, handle and progress clears.
 */
static void mrsUpdateClear(void)
{
	// Line clear check is delayed by the clear offset
	if (mrsTet.player.state == PlayerSpawn &&
		mrsTet.player.spawnDelay + 1 == ClearOffset) {
		int clearedCount = checkClears();
		if (clearedCount) {
			mrsTet.player.state = PlayerClear;
			mrsTet.player.clearDelay = 0;
		}
	}

	// Advance counter, switch back to spawn delay if elapsed
	if (mrsTet.player.state == PlayerClear) {
		mrsTet.player.clearDelay += 1;
		if (mrsTet.player.clearDelay > ClearDelay) {
			thump();
			mrsTet.player.state = PlayerSpawn;
		}
	}
}

/**
 * Spawn a new piece if needed.
 */
static void mrsUpdateSpawn(void)
{
	if (mrsTet.state != TetrionPlaying || debugPauseSpawn)
		return; // Do not spawn during countdown or gameover
	if (mrsTet.player.state == PlayerSpawn || mrsTet.player.state == PlayerNone) {
		mrsTet.player.spawnDelay += 1;
		if (mrsTet.player.spawnDelay >= SpawnDelay)
			spawnPiece();
	}
}

/**
 * Move player piece down through gravity or manual dropping.
 */
static void mrsUpdateGravity(void)
{
	if (mrsTet.state == TetrionOutro)
		return; // Prevent zombie blocks
	if (mrsTet.player.state != PlayerSpawned && mrsTet.player.state != PlayerActive)
		return;

	int remainingGravity = mrsTet.player.gravity;
	if (mrsTet.player.state == PlayerActive) {
		if (inputHeld(InputDown) || inputHeld(InputUp))
			remainingGravity = FieldHeight * SubGrid;
	}

	if (canDrop()) // Queue up the gravity drops
		mrsTet.player.ySub += remainingGravity;
	else
		mrsTet.player.ySub = 0;

	while (mrsTet.player.ySub >= SubGrid) { // Drop until queue empty
		drop();
		mrsTet.player.ySub -= SubGrid;
	}

	// Hard drop
	if (mrsTet.player.state == PlayerActive) {
		if (inputHeld(InputDown))
			lock();
	}
}

/**
 * Lock player piece by lock delay expiry or manual lock.
 */
static void mrsUpdateLocking(void)
{
	if (mrsTet.player.state != PlayerActive || mrsTet.state != TetrionPlaying)
		return;
	if (canDrop())
		return;

	if (!debugInfLock)
		mrsTet.player.lockDelay += 1;
	// Two sources of locking: lock delay expired, manlock
	if (mrsTet.player.lockDelay > LockDelay || inputHeld(InputDown))
		lock();
}

/**
 * Win the game. Try to get this function called while playing.
 */
static void mrsUpdateWin(void)
{
	//TODO
}

void mrsAdvance(darray* inputs)
{
	assert(inputs);
	assert(initialized);

	mrsUpdateInputs(inputs);
	mrsUpdateState();
	mrsUpdateRotation();
	mrsUpdateShift();
	mrsUpdateClear();
	mrsUpdateSpawn();
	mrsUpdateGravity();
	mrsUpdateLocking();
	mrsUpdateWin();
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Create some pretty particle effects on line clear. Call this before the row
 * is actually cleared.
 * @param row Height of the cleared row
 * @param power Number of lines cleared, which affects the strength
 */
static void genParticlesClear(int row, int power)
{
	for (int x = 0; x < FieldWidth; x += 1) {
		for (int ySub = 0; ySub < 8; ySub += 1) {
			color4 cellColor = minoColor(
				fieldGet(mrsTet.field, (point2i){x, row}));
			color4Copy(particlesClear.color, cellColor);
			particlesClear.color.r *= ParticlesClearBoost;
			particlesClear.color.g *= ParticlesClearBoost;
			particlesClear.color.b *= ParticlesClearBoost;
			particlesClear.distanceMin = 3.2f * power;
			particlesClear.distanceMax = particlesClear.distanceMin * 2.0f;
			particlesGenerate((point3f){
					(float)x - (float)FieldWidth / 2,
					(float)row + 0.0625f + 0.125f * (float)ySub,
					0.0f},
				power, &particlesClear);
		}
	}
}

/**
 * Create a dust cloud effect on blocks that have fallen on top of other blocks.
 * Use after the relevant fieldDropRow() was called.
 * @param row Position of the row that was cleared
 */
static void genParticlesThump(int row)
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

/**
 * Create a dust cloud effect under the player piece. Use after drop().
 */
static void genParticlesDrop(void)
{
	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = mrsTet.player.pos.x + (*playerPiece)[i].x;
		int y = mrsTet.player.pos.y + (*playerPiece)[i].y;
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
 * Create a friction effect under the player piece as it moves sideways.
 * Use after shift().
 * @param direction Most recently performed shift. -1 for left, 1 for right
 * @param fast true if DAS shift, false if manual
 */
static void genParticlesSlide(int direction, bool fast)
{
	ParticleParams* params = fast? &particlesSlideFast : &particlesSlide;
	params->directionHorz = direction;

	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = mrsTet.player.pos.x + (*playerPiece)[i].x;
		int y = mrsTet.player.pos.y + (*playerPiece)[i].y;
		if (fieldGet(mrsTet.field, (point2i){x, y - 1})) {
			particlesGenerate((point3f){
				(float)x - (float)FieldWidth / 2,
				(float)y,
				0.0f
			}, 8, params);
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
	// A bit out of place here, but no need to get this more than once
	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);

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
		tint->r *= FieldDim;
		tint->g *= FieldDim;
		tint->b *= FieldDim;
		if (y >= FieldHeightVisible)
			tint->a *= ExtraRowDim;

		bool playerCell = false;
		for (size_t j = 0; j < MinosPerPiece; j += 1) {
			int px = (*playerPiece)[j].x + mrsTet.player.pos.x;
			int py = (*playerPiece)[j].y + mrsTet.player.pos.y;
			if (x == px && y == py) {
				playerCell = true;
				break;
			}
		}
		if (playerCell) {
			float flash = tweenApply(&lockFlash);
			color4Copy(*highlight,
				((color4){LockFlashBrightness, LockFlashBrightness,
				          LockFlashBrightness, flash}));
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
	if (mrsTet.player.state != PlayerActive &&
		mrsTet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + mrsTet.player.pos.x;
		float y = (*playerPiece)[i].y + mrsTet.player.pos.y;

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

		color4Copy(*tint, minoColor(mrsTet.player.type));
		if (!canDrop()) {
			tweenRestart(&lockDim);
			lockDim.start -= mrsTet.player.lockDelay * MrsUpdateTick;
			float dim = tweenApply(&lockDim);
			tint->r *= dim;
			tint->g *= dim;
			tint->b *= dim;
		}
		color4Copy(*highlight, Color4Clear);
		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
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

	piece* playerPiece = mrsGetPiece(mrsTet.player.type, mrsTet.player.rotation);
	point2i ghostPos = mrsTet.player.pos;
	while (!pieceOverlapsField(playerPiece, (point2i){
		ghostPos.x,
		ghostPos.y - 1
	}, mrsTet.field))
		ghostPos.y -= 1; // Drop down as much as possible

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + ghostPos.x;
		float y = (*playerPiece)[i].y + ghostPos.y;

		color4* tint = darrayProduce(blockTintsAlpha);
		color4* highlight = darrayProduce(blockHighlightsAlpha);
		mat4x4* transform = darrayProduce(blockTransformsAlpha);

		color4Copy(*tint, minoColor(mrsTet.player.type));
		tint->a *= GhostDim;
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
	piece* previewPiece = mrsGetPiece(mrsTet.player.preview, SpinNone);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*previewPiece)[i].x + PreviewX;
		float y = (*previewPiece)[i].y + PreviewY;
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
		float alpha = BorderDim;
		if (y >= FieldHeightVisible)
			alpha *= ExtraRowDim;

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

void mrsDebug(void)
{
	if (nk_begin(nkCtx(), "MRS debug", nk_rect(30, 30, 200, 180),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE
			| NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(nkCtx(), 0, 2);
		nk_labelf(nkCtx(), NK_TEXT_CENTERED, "Gravity: %d.%02x",
			mrsTet.player.gravity / SubGrid, mrsTet.player.gravity % SubGrid);
		nk_slider_int(nkCtx(), 4, &mrsTet.player.gravity, SubGrid * 20, 4);
		nk_layout_row_dynamic(nkCtx(), 0, 1);
		nk_checkbox_label(nkCtx(), "Pause spawning", &debugPauseSpawn);
		nk_checkbox_label(nkCtx(), "Infinite lock delay", &debugInfLock);
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
		for (int y = FieldHeightVisible - 1; y >= 0; y -= 1) {
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
