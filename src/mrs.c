/**
 * Implementation of mrs.h
 * @file
 */

#include "mrs.h"

#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "mrstables.h"
#include "renderer.h"
#include "particles.h"
#include "mapper.h"
#include "debug.h"
#include "model.h"
#include "world.h"
#include "util.h"
#include "ease.h"
#include "log.h"

#define FieldWidth 10u ///< Width of #field
#define FieldHeight 22u ///< Height of #field

#define SpawnX 3 ///< X position of player piece spawn
#define SpawnY 18 ///< Y position of player piece spawn
#define SubGrid 256 ///< Number of subpixels per cell, used for gravity

#define StartingTokens 9 ///< Number of tokens that each piece starts with

#define AutoshiftCharge 16 ///< Frames direction has to be held before autoshift
#define AutoshiftRepeat 1 ///< Frames between autoshifts
#define LockDelay 30 ///< Frames a piece can spend on the stack before locking
#define ClearOffset 4 ///< Frames between piece lock and line clear
#define ClearDelay 41 ///< Frames between line clear and thump
#define SpawnDelay 30 ///< Frames between lock/thumb and new piece spawn

/// State of player piece FSM
typedef enum PlayerState {
	PlayerNone, ///< zero value
	PlayerSpawned, ///< the frame of piece spawn
	PlayerActive, ///< piece can be controlled
	PlayerClear, ///< line has been cleared
	PlayerSpawn, ///< waiting to spawn a new piece
	PlayerSize ///< terminator
} PlayerState;

/// A player-controlled active piece
typedef struct Player {
	bool inputMapRaw[InputSize]; ///< Unfiltered input state
	bool inputMap[InputSize]; ///< Filtered input state
	bool inputMapPrev[InputSize]; ///< #inputMap of the previous frame
	InputType lastDirection; ///< None, Left or Right

	PlayerState state;
	mino type; ///< Current player piece
	mino preview; ///< Next player piece
	int tokens[MinoGarbage - 1]; ///< Past player pieces
	spin rotation; ///< ::spin of current piece
	point2i pos; ///< Position of current piece
	int ySub; ///< Y subgrid of current piece
	int yLowest; ///< The bottommost row reached by current piece

	int autoshiftDirection; ///< Autoshift state: -1 left, 1 right, 0 none
	int autoshiftCharge;
	int autoshiftDelay;
	int lockDelay;
	int clearDelay;
	int spawnDelay;
	int gravity;
} Player;

/// State of tetrion FSM
typedef enum TetrionState {
	TetrionNone, ///< zero value
	TetrionReady, ///< intro
	TetrionPlaying, ///< gameplay
	TetrionOutro, ///< outro
	TetrionSize ///< terminator
} TetrionState;

/// A play's logical state
typedef struct Tetrion {
	TetrionState state;
	int ready; ///< Countdown timer
	int frame; ///< Frame counter since #ready = 0

	Field* field;
	bool linesCleared[FieldHeight]; ///< Storage for line clears pending a thump
	Player player;
	Rng* rng;
} Tetrion;

/// Full state of the mode
static Tetrion tet = {0};

// Debug switches
static int debugPauseSpawn = 0; // Boolean, int for compatibility
static int debugInfLock = 0; // Boolean, int for compatibility

static bool initialized = false;

/**
 * Test whether an input has been pressed just now.
 * @param type ::InputType to test
 * @return true if just pressed, false otherwise
 */
#define inputPressed(type) \
    (tet.player.inputMap[(type)] && !tet.player.inputMapPrev[(type)])

/**
 * Test whether an input is pressed during this frame.
 * @param type ::InputType to test
 * @return true if pressed, false if not pressed
 */
#define inputHeld(type) \
    (tet.player.inputMap[(type)])

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

/// Player piece animation after the piece locks
static Ease lockFlash = {
	.from = 1.0f,
	.to = 0.0f,
	.duration = 8 * MrsUpdateTick,
	.type = EaseLinear
};

/// Player piece animation as the lock delay ticks down
static Ease lockDim = {
	.from = 1.0f,
	.to = 0.4f,
	.duration = LockDelay * MrsUpdateTick,
	.type = EaseLinear
};

/// Animation of the scene when combo counter changes
static Ease comboFade = {
	.from = 1.1f,
	.to = 1.1f,
	.duration = 24 * MrsUpdateTick,
	.type = EaseOutQuadratic
};

static ParticleParams particlesClear = {
	.color = {0.0f, 0.0f, 0.0f, 1.0f},
	.durationMin = secToNsec(0),
	.durationMax = secToNsec(2),
	.radius = 64.0f,
	.power = 5.0f,
	.directionVert = 0,
	.ease = EaseOutExponential
};

static ParticleParams particlesThump = {
	.color = {0.5f, 0.5f, 0.5f, 1.0f},
	.durationMin = secToNsec(1)/2,
	.durationMax = secToNsec(1),
	.radius = 8.0f,
	.power = 2.0f,
	.directionVert = 1,
	.ease = EaseOutExponential
};

static void genParticlesClear(int row, int power);
static void genParticlesThump(int row);

/**
 * Try to kick the player piece into a legal position.
 * @param prevRotation piece rotation prior to the kick
 * @return true if player piece already legal or successfully kicked, false if
 * no kick was possible
 */
static bool tryKicks(spin prevRotation)
{
	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // Original position

	if (tet.player.state == PlayerSpawned)
		return false; // If this is IRS, don't attempt kicks
	if (tet.player.type == MinoI)
		return false; // I doesn't kick

	// L/J/T floorkick
	if ((tet.player.type == MinoL ||
		tet.player.type == MinoJ ||
		tet.player.type == MinoT) &&
		prevRotation == Spin180) {
		tet.player.pos.y += 1;
		if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
			return true; // 1 to the right
		tet.player.pos.y -= 1;
	}

	// Now that every exception is filtered out, we can try the default kicks
	int preference = tet.player.lastDirection == InputRight ? 1 : -1;

	// Down
	tet.player.pos.y -= 1;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the right
	tet.player.pos.y += 1;

	// Left/right
	tet.player.pos.x += preference;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the right
	tet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the left
	tet.player.pos.x += preference;

	// Down+left/right
	tet.player.pos.y -= 1;
	tet.player.pos.x += preference;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the right
	tet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the left
	tet.player.pos.x += preference;
	tet.player.pos.y += 1;

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
	spin prevRotation = tet.player.rotation;
	point2i prevPosition = tet.player.pos;

	if (direction == 1)
		spinClockwise(&tet.player.rotation);
	else
		spinCounterClockwise(&tet.player.rotation);

	// Apply crawl offsets to I,S,Z
	if (tet.player.type == MinoI ||
		tet.player.type == MinoS ||
		tet.player.type == MinoZ) {
		if (direction == -1 &&
			(prevRotation == Spin90 ||
				prevRotation == Spin270))
			tet.player.pos.x += 1;
		if (direction == 1 &&
			(prevRotation == SpinNone ||
				prevRotation == Spin180)) {
			tet.player.pos.x -= 1;
		}
	}

	if (!tryKicks(prevRotation)) {
		tet.player.rotation = prevRotation;
		tet.player.pos.x = prevPosition.x;
		tet.player.pos.y = prevPosition.y;
	}
}

/**
 * Attempt to shift the player piece in the given direction.
 * @param direction -1 for left, 1 for right
 */
static void shift(int direction)
{
	assert(direction == 1 || direction == -1);
	tet.player.pos.x += direction;
	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	if (pieceOverlapsField(playerPiece, tet.player.pos, tet.field)) {
		tet.player.pos.x -= direction;
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
	for (size_t i = 0; i < countof(tet.player.tokens); i += 1)
		if (tet.player.tokens[i] > 0) tokenTotal += tet.player.tokens[i];
	assert(tokenTotal);

	// Create and fill the token list
	int tokenList[tokenTotal];
	size_t tokenListIndex = 0;
	for (size_t i = 0; i < countof(tet.player.tokens); i += 1) {
		if (tet.player.tokens[i] <= 0) continue;
		for (size_t j = 0; j < tet.player.tokens[i]; j += 1) {
			assert(tokenListIndex < tokenTotal);
			tokenList[tokenListIndex] = i;
			tokenListIndex += 1;
		}
	}
	assert(tokenListIndex == tokenTotal);

	// Pick a random token from the list and update the token distribution
	int picked = tokenList[rngInt(tet.rng, tokenTotal)];
	for (size_t i = 0; i < countof(tet.player.tokens); i += 1) {
		if (i == picked)
			tet.player.tokens[i] -= countof(tet.player.tokens) - 1;
		else
			tet.player.tokens[i] += 1;
	}

	return picked + MinoNone + 1;
}

/**
 * Stop the round.
 */
static void gameOver(void)
{
	tet.state = TetrionOutro;
}

/**
 * Prepare the player piece for a brand new adventure at the top of the field.
 */
static void spawnPiece(void)
{
	tet.player.state = PlayerSpawned; // Some moves restricted on first frame
	tet.player.pos.x = SpawnX;
	tet.player.pos.y = SpawnY;
	tet.player.yLowest = tet.player.pos.y;

	// Picking the next piece
	tet.player.type = tet.player.preview;
	tet.player.preview = randomPiece();

	if (tet.player.type == MinoI)
		tet.player.pos.y -= 1; // I starts lower than other pieces
	tet.player.ySub = 0;
	tet.player.lockDelay = 0;
	tet.player.spawnDelay = 0;
	tet.player.clearDelay = 0;
	tet.player.rotation = SpinNone;

	// IRS
	if (inputHeld(InputButton2)) {
		rotate(-1);
	} else {
		if (inputHeld(InputButton1) || inputHeld(InputButton3))
			rotate(1);
	}

	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	if (pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		gameOver();

	// Increase gravity
	if (tet.player.gravity >= 20 * SubGrid) return;
	int level = tet.player.gravity / 64 + 1;
	tet.player.gravity += level;
}

/**
 * Check if field row for full lines and initiate clears.
 * @return Number of lines cleared
 */
static int checkClears(void)
{
	int count = 0;
	for (int y = 0; y < FieldHeight; y += 1) {
		if (!fieldIsRowFull(tet.field, y))
			continue;
		count += 1;
		tet.linesCleared[y] = true;
	}
	for (int y = 0; y < FieldHeight; y += 1) {
		if (!tet.linesCleared[y]) continue;
		genParticlesClear(y, count);
		fieldClearRow(tet.field, y);
	}
	return count;
}

/**
 * "Thump" previously cleared lines, bringing them crashing into the ground.
 */
static void thump(void)
{
	for (int y = FieldHeight - 1; y >= 0; y -= 1) {
		if (!tet.linesCleared[y])
			continue; // Drop only above cleared lines
		fieldDropRow(tet.field, y);
		tet.linesCleared[y] = false;
		genParticlesThump(y);
	}
}

/**
 * Check whether the player piece could move down one cell without overlapping
 * the field.
 * @return true if no overlap, false if overlap
 */
static bool canDrop(void)
{
	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	return !pieceOverlapsField(playerPiece, (point2i){
		.x = tet.player.pos.x,
		.y = tet.player.pos.y - 1
	}, tet.field);
}

/**
 * Move the player piece down one cell if possible, also calculating other
 * appropriate values.
 */
static void drop(void)
{
	if (!canDrop())
		return;

	tet.player.pos.y -= 1;

	// Reset lock delay if piece dropped lower than ever
	if (tet.player.pos.y < tet.player.yLowest) {
		tet.player.lockDelay = 0;
		tet.player.yLowest = tet.player.pos.y;
	}
}

/**
 * Stamp player piece onto the grid.
 */
static void lock(void)
{
	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	fieldStampPiece(tet.field, playerPiece, tet.player.pos, tet.player.type);
	tet.player.state = PlayerSpawn;
	easeRestart(&lockFlash);
}

void mrsInit(void)
{
	if (initialized) return;

	// Logic init
	structClear(tet);
	tet.frame = -1;
	tet.ready = 3 * 50;
	tet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	tet.player.autoshiftDelay = AutoshiftRepeat; // Starts out pre-charged
	tet.player.spawnDelay = SpawnDelay; // Start instantly
	tet.player.gravity = 3;

	tet.rng = rngCreate((uint64_t)time(null));
	for (size_t i = 0; i < countof(tet.player.tokens); i += 1)
		tet.player.tokens[i] = StartingTokens;
	tet.player.preview = randomPiece();

	tet.state = TetrionReady;

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
	rngDestroy(tet.rng);
	tet.rng = null;
	fieldDestroy(tet.field);
	tet.field = null;
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
	if (tet.state != TetrionOutro) {
		for (size_t i = 0; i < darraySize(inputs); i += 1) {
			Input* in = darrayGet(inputs, i);
			assert(in->type < InputSize);
			tet.player.inputMapRaw[in->type] = in->state;
		}
	} else { // Force-release everything on gameover
		arrayClear(tet.player.inputMapRaw);
	}

	// Rotate the input arrays
	arrayCopy(tet.player.inputMapPrev, tet.player.inputMap);
	arrayCopy(tet.player.inputMap, tet.player.inputMapRaw);

	// Filter conflicting inputs
	if (tet.player.inputMap[InputDown] || tet.player.inputMap[InputUp]) {
		tet.player.inputMap[InputLeft] = false;
		tet.player.inputMap[InputRight] = false;
	}
	if (tet.player.inputMap[InputLeft] && tet.player.inputMap[InputRight]) {
		if (tet.player.lastDirection == InputLeft)
			tet.player.inputMap[InputRight] = false;
		if (tet.player.lastDirection == InputRight)
			tet.player.inputMap[InputLeft] = false;
	}
}

/**
 * Check for state triggers and progress through states.
 */
static void mrsUpdateState(void)
{
	if (tet.state == TetrionReady) {
		tet.ready -= 1;
		if (tet.ready == 0)
			tet.state = TetrionPlaying;
	} else if (tet.state == TetrionPlaying) {
		tet.frame += 1;
	}
	if (tet.player.state == PlayerSpawned)
		tet.player.state = PlayerActive;
}

/**
 * Spin the player piece.
 */
static void mrsUpdateRotation(void)
{
	if (tet.player.state != PlayerActive)
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
	if (!shiftDirection || shiftDirection != tet.player.autoshiftDirection) {
		tet.player.autoshiftDirection = shiftDirection;
		tet.player.autoshiftCharge = 0;
		tet.player.autoshiftDelay = AutoshiftRepeat; // Starts out pre-charged
		if (shiftDirection && tet.player.state == PlayerActive)
			shift(shiftDirection);
	}

	// If moving, advance and apply DAS
	if (!shiftDirection)
		return;
	if (tet.player.autoshiftCharge < AutoshiftCharge)
		tet.player.autoshiftCharge += 1;
	if (tet.player.autoshiftCharge == AutoshiftCharge) {
		if (tet.player.autoshiftDelay < AutoshiftRepeat)
			tet.player.autoshiftDelay += 1;

		// If during ARE, keep the DAS charged
		if (tet.player.autoshiftDelay >= AutoshiftRepeat
			&& tet.player.state == PlayerActive) {
			tet.player.autoshiftDelay = 0;
			shift(tet.player.autoshiftDirection);
		}
	}
}

/**
 * Check for cleared lines, handle and progress clears.
 */
static void mrsUpdateClear(void)
{
	// Line clear check is delayed by the clear offset
	if (tet.player.state == PlayerSpawn &&
		tet.player.spawnDelay + 1 == ClearOffset) {
		int clearedCount = checkClears();
		if (clearedCount) {
			tet.player.state = PlayerClear;
			tet.player.clearDelay = 0;
		}
	}

	// Advance counter, switch back to spawn delay if elapsed
	if (tet.player.state == PlayerClear) {
		tet.player.clearDelay += 1;
		if (tet.player.clearDelay > ClearDelay) {
			thump();
			tet.player.state = PlayerSpawn;
		}
	}
}

/**
 * Spawn a new piece if needed.
 */
static void mrsUpdateSpawn(void)
{
	if (tet.state != TetrionPlaying || debugPauseSpawn)
		return; // Do not spawn during countdown or gameover
	if (tet.player.state == PlayerSpawn || tet.player.state == PlayerNone) {
		tet.player.spawnDelay += 1;
		if (tet.player.spawnDelay >= SpawnDelay)
			spawnPiece();
	}
}

/**
 * Move player piece down through gravity or manual dropping.
 */
static void mrsUpdateGravity(void)
{
	if (tet.state == TetrionOutro)
		return; // Prevent zombie blocks
	if (tet.player.state != PlayerSpawned && tet.player.state != PlayerActive)
		return;

	int remainingGravity = tet.player.gravity;
	if (tet.player.state == PlayerActive) {
		if (inputHeld(InputDown) || inputHeld(InputUp))
			remainingGravity = FieldHeight * SubGrid;
	}

	if (canDrop()) // Queue up the gravity drops
		tet.player.ySub += remainingGravity;
	else
		tet.player.ySub = 0;

	while (tet.player.ySub >= SubGrid) { // Drop until queue empty
		drop();
		tet.player.ySub -= SubGrid;
	}

	// Hard drop
	if (tet.player.state == PlayerActive) {
		if (inputHeld(InputDown))
			lock();
	}
}

/**
 * Lock player piece by lock delay expiry or manual lock.
 */
static void mrsUpdateLocking(void)
{
	if (tet.player.state != PlayerActive || tet.state != TetrionPlaying)
		return;
	if (canDrop())
		return;

	if (!debugInfLock)
		tet.player.lockDelay += 1;
	// Two sources of locking: lock delay expired, manlock
	if (tet.player.lockDelay > LockDelay || inputHeld(InputDown))
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
				fieldGet(tet.field, (point2i){x, row}));
			color4Copy(particlesClear.color, cellColor);
			particlesClear.color.r *= 2.0f;
			particlesClear.color.g *= 2.0f;
			particlesClear.color.b *= 2.0f;
			if (power == 4) {
				particlesClear.durationMin = secToNsec(1);
				particlesClear.ease = EaseInOutExponential;
			} else {
				particlesClear.durationMin = secToNsec(0);
				particlesClear.ease = EaseOutExponential;
			}
			particlesClear.power = 5.0f * power;
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
		if (fieldGet(tet.field, (point2i){x, row})
			&& fieldGet(tet.field, (point2i){x, row - 1}))
		particlesGenerate((point3f){
			(float)x - (float)FieldWidth / 2,
			(float)row,
			0.0f
		}, 8, &particlesThump);
	}
}
/**
 * Draw the scene model, which visually wraps the tetrion field.
 */
static void mrsDrawScene(void)
{
	float boost = easeApply(&comboFade);
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
	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);

	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;
		mino type = fieldGet(tet.field, (point2i){x, y});
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
			int px = (*playerPiece)[j].x + tet.player.pos.x;
			int py = (*playerPiece)[j].y + tet.player.pos.y;
			if (x == px && y == py) {
				playerCell = true;
				break;
			}
		}
		if (playerCell) {
			float flash = easeApply(&lockFlash);
			color4Copy(*highlight,
				((color4){LockFlashBrightness, LockFlashBrightness,
				          LockFlashBrightness, flash}));
		} else {
			color4Copy(*highlight, Color4Clear);
		}

		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
	}
}

/**
 * Queue the player piece on top of the field.
 */
static void mrsQueuePlayer(void)
{
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + tet.player.pos.x;
		float y = (*playerPiece)[i].y + tet.player.pos.y;

		color4* tint = null;
		color4* highlight = null;
		mat4x4* transform = null;
		if (minoColor(tet.player.type).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			highlight = darrayProduce(blockHighlightsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			highlight = darrayProduce(blockHighlightsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(tet.player.type));
		if (!canDrop()) {
			easeRestart(&lockDim);
			lockDim.start -= tet.player.lockDelay * MrsUpdateTick;
			float dim = easeApply(&lockDim);
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
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = mrsGetPiece(tet.player.type, tet.player.rotation);
	point2i ghostPos = tet.player.pos;
	while (!pieceOverlapsField(playerPiece, (point2i){
		ghostPos.x,
		ghostPos.y - 1
	}, tet.field))
		ghostPos.y -= 1; // Drop down as much as possible

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + ghostPos.x;
		float y = (*playerPiece)[i].y + ghostPos.y;

		color4* tint = darrayProduce(blockTintsAlpha);
		color4* highlight = darrayProduce(blockHighlightsAlpha);
		mat4x4* transform = darrayProduce(blockTransformsAlpha);

		color4Copy(*tint, minoColor(tet.player.type));
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
	if (tet.player.preview == MinoNone)
		return;
	piece* previewPiece = mrsGetPiece(tet.player.preview, SpinNone);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*previewPiece)[i].x + PreviewX;
		float y = (*previewPiece)[i].y + PreviewY;
		if (tet.player.preview == MinoI)
			y -= 1;

		color4* tint = null;
		color4* highlight = null;
		mat4x4* transform = null;
		if (minoColor(tet.player.preview).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			highlight = darrayProduce(blockHighlightsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			highlight = darrayProduce(blockHighlightsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(tet.player.preview));
		color4Copy(*highlight, Color4Clear);
		mat4x4_translate(*transform, x, y, 0.0f);
	}
}

/**
 * Draw all queued blocks with alpha pre-pass.
 */
static void mrsDrawQueuedBlocks(void)
{
	modelDraw(block, darraySize(blockTransformsOpaque),
		darrayData(blockTintsOpaque),
		darrayData(blockHighlightsOpaque),
		darrayData(blockTransformsOpaque));
	darrayClear(blockTintsOpaque);
	darrayClear(blockHighlightsOpaque);
	darrayClear(blockTransformsOpaque);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Depth prepass start
	modelDraw(block, darraySize(blockTransformsAlpha),
		darrayData(blockTintsAlpha),
		darrayData(blockHighlightsAlpha),
		darrayData(blockTransformsAlpha));
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Depth prepass end
	modelDraw(block, darraySize(blockTransformsAlpha),
		darrayData(blockTintsAlpha),
		darrayData(blockHighlightsAlpha),
		darrayData(blockTransformsAlpha));
	darrayClear(blockTintsAlpha);
	darrayClear(blockHighlightsAlpha);
	darrayClear(blockTransformsAlpha);
}

static void mrsBorderQueue(point3f pos, size3f size, color4 color)
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
	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;

		if (!fieldGet(tet.field, (point2i){x, y})) continue;

		// Coords transformed to world space
		float tx = x - (signed)(FieldWidth / 2);
		float ty = y;
		float alpha = BorderDim;
		if (y >= FieldHeightVisible)
			alpha *= ExtraRowDim;

		// Left
		if (!fieldGet(tet.field, (point2i){x - 1, y}))
			mrsBorderQueue((point3f){tx, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Right
		if (!fieldGet(tet.field, (point2i){x + 1, y}))
			mrsBorderQueue((point3f){tx + 0.875f, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down
		if (!fieldGet(tet.field, (point2i){x, y - 1}))
			mrsBorderQueue((point3f){tx + 0.125f, ty, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up
		if (!fieldGet(tet.field, (point2i){x, y + 1}))
			mrsBorderQueue((point3f){tx + 0.125f, ty + 0.875f, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Left
		if (!fieldGet(tet.field, (point2i){x - 1, y - 1})
			|| !fieldGet(tet.field, (point2i){x - 1, y})
			|| !fieldGet(tet.field, (point2i){x, y - 1}))
			mrsBorderQueue((point3f){tx, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Right
		if (!fieldGet(tet.field, (point2i){x + 1, y - 1})
			|| !fieldGet(tet.field, (point2i){x + 1, y})
			|| !fieldGet(tet.field, (point2i){x, y - 1}))
			mrsBorderQueue((point3f){tx + 0.875f, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Left
		if (!fieldGet(tet.field, (point2i){x - 1, y + 1})
			|| !fieldGet(tet.field, (point2i){x - 1, y})
			|| !fieldGet(tet.field, (point2i){x, y + 1}))
			mrsBorderQueue((point3f){tx, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Right
		if (!fieldGet(tet.field, (point2i){x + 1, y + 1})
			|| !fieldGet(tet.field, (point2i){x + 1, y})
			|| !fieldGet(tet.field, (point2i){x, y + 1}))
			mrsBorderQueue((point3f){tx + 0.875f, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
	}

	modelDraw(border, darraySize(borderTransforms),
		darrayData(borderTints), null,
		darrayData(borderTransforms));
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
			tet.player.gravity / SubGrid, tet.player.gravity % SubGrid);
		nk_slider_int(nkCtx(), 4, &tet.player.gravity, SubGrid * 20, 4);
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
				mino cell = fieldGet(tet.field, (point2i){x, y});
				color4 cellColor = minoColor(cell);
				if (nk_button_color(nkCtx(), nk_rgba(
					cellColor.r * 255.0f,
					cellColor.g * 255.0f,
					cellColor.b * 255.0f,
					cellColor.a * 255.0f))) {
					if (cell)
						fieldSet(tet.field, (point2i){x, y}, MinoNone);
					else
						fieldSet(tet.field, (point2i){x, y}, MinoGarbage);
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
	worldSetAmbientColor((color3){0.0185f, 0.029f, 0.0944f});
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
