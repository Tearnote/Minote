/**
 * Implementation of halfrs.h
 * @file
 */

#include "halfrs.h"

#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "halfrstables.h"
#include "renderer.h"
#include "particles.h"
#include "mapper.h"
#include "model.h"
#include "world.h"
#include "util.h"
#include "ease.h"
#include "log.h"

#define FieldWidth 10u ///< Width of #field
#define FieldHeight 23u ///< Height of #field

#define SpawnX 3 ///< X position of player piece spawn
#define SpawnY 17 ///< Y position of player piece spawn
#define SubGrid 256 ///< Number of subpixels per cell, used for gravity

#define HistorySize 4 ///< Number of past pieces to remember for rng bias
#define MaxRerolls 4 ///< Number of times the rng avoids pieces from history

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
	spin rotation; ///< ::spin of current piece
	HalfrsPoint pos; ///< Position of current piece
	int ySub; ///< Y subgrid of current piece

	int autoshiftDirection; ///< Autoshift state: -1 left, 1 right, 0 none
	int autoshiftCharge;
	int autoshiftDelay;
	int lockDelay;
	int clearDelay;
	int spawnDelay;
	int gravity;

	darray* sixBag;
	darray* sevenBag;
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
	.length = 8 * HalfrsUpdateTick,
	.type = EaseLinear
};

/// Player piece animation as the lock delay ticks down
static Ease lockDim = {
	.from = 1.0f,
	.to = 0.4f,
	.length = LockDelay * HalfrsUpdateTick,
	.type = EaseLinear
};

/// Convert combo to highlight multiplier
#define comboHighlight(combo) \
    (1.1f + 0.025f * (combo))

/// Animation of the scene when combo counter changes
static Ease comboFade = {
	.from = comboHighlight(1),
	.to = comboHighlight(1),
	.length = 24 * HalfrsUpdateTick,
	.type = EaseOutQuadratic
};

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

/**
 * Offset the #target ::HalfrsPoint by the #source value.
 * @param target point being added to
 * @param source offset being added
 */
static void halfrsPointAdd(HalfrsPoint* target, HalfrsPoint source)
{
	target->x += source.x;
	target->y += source.y;
	if (source.xHalf) {
		if (!target->xHalf) {
			target->xHalf = true;
		} else {
			target->xHalf = false;
			target->x += 1;
		}
	}
	if (source.yHalf) {
		if (!target->yHalf) {
			target->yHalf = true;
		} else {
			target->yHalf = false;
			target->y += 1;
		}
	}
}

/**
 * Check if a piece overlaps the field, taking into account MRS half-offset.
 * @return true if there is overlap, false if not
 */
static bool isOverlap(piece* p, HalfrsPoint pos)
{
	point2i integerPos = *(point2i*)&pos;
	
	if (pieceOverlapsField(p, integerPos, tet.field)) return true;
	
	if (pos.xHalf) {
		integerPos.x += 1;
		if (pieceOverlapsField(p, integerPos, tet.field)) return true;
		integerPos.x -= 1;
	}
	
	if (pos.yHalf) {
		integerPos.y += 1;
		if (pieceOverlapsField(p, integerPos, tet.field)) return true;
		integerPos.y -= 1;
	}

	if (pos.xHalf && pos.yHalf) {
		integerPos.x += 1;
		integerPos.y += 1;
		if (pieceOverlapsField(p, integerPos, tet.field)) return true;
	}

	return false;
}

/**
 * Try to kick the player piece into a legal position without changing
 * its height.
 * @return true if player piece already legal or successfully kicked, false if
 * no kick was possible
 */
static bool tryHorizontalKicks(void)
{
	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);
	HalfrsPoint original = tet.player.pos;

	// Original position
	if (!isOverlap(playerPiece, original)) return true;

	// Set up the preferred direction
	HalfrsPoint offset = {0};
	offset.xHalf = true;
	if (tet.player.lastDirection == InputLeft)
		offset.x = -1;

	// Check kicks in preferred direction
	for (size_t i = 0; i < 3; i += 1) {
		halfrsPointAdd(&tet.player.pos, offset);
		if (!isOverlap(playerPiece, tet.player.pos)) return true;
	}
	tet.player.pos.x = original.x;
	tet.player.pos.xHalf = original.xHalf;

	// Check kicks in opposite direction
	if (offset.x == 0)
		offset.x = -1;
	else
		offset.x = 0;

	for (size_t i = 0; i < 3; i += 1) {
		halfrsPointAdd(&tet.player.pos, offset);
		if (!isOverlap(playerPiece, tet.player.pos)) return true;
	}
	tet.player.pos.x = original.x;
	tet.player.pos.xHalf = original.xHalf;

	return false;
}

/**
 * Try to kick the player piece into a legal position.
 * @return true if player piece already legal or successfully kicked, false if
 * no kick was possible
 */
static bool tryKicks(void)
{
	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);

	// Original position
	if (!isOverlap(playerPiece, tet.player.pos)) return true;

	// If this is IRS, don't attempt kicks
	if (tet.player.state == PlayerSpawned) return false;

	if (tet.player.pos.yHalf) {
		// We try half a grid lower
		tet.player.pos.yHalf = false;
		if(tryHorizontalKicks()) return true;
		// And again half a grid higher
		tet.player.pos.y += 1;
		if(tryHorizontalKicks()) return true;
		// Failed, restore original position
		tet.player.pos.y -= 1;
		tet.player.pos.yHalf = true;
		return false;
	}
	return tryHorizontalKicks();
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
	HalfrsPoint prevPos = tet.player.pos;

	if (direction == 1)
		spinClockwise(&tet.player.rotation);
	else
		spinCounterClockwise(&tet.player.rotation);

	HalfrsPoint prevOffset = halfrsGetPieceOffset(tet.player.type, prevRotation);
	HalfrsPoint newOffset = halfrsGetPieceOffset(tet.player.type, tet.player.rotation);
	tet.player.pos.x += newOffset.x - prevOffset.x;
	tet.player.pos.y += newOffset.y - prevOffset.y;
	if (!prevOffset.xHalf && newOffset.xHalf) { // Move half to the right
		if (tet.player.pos.xHalf) {
			tet.player.pos.x += 1;
			tet.player.pos.xHalf = false;
		} else {
			tet.player.pos.xHalf = true;
		}
	}
	if (prevOffset.xHalf && !newOffset.xHalf) { // Move half to the left
		if (tet.player.pos.xHalf) {
			tet.player.pos.xHalf = false;
		} else {
			tet.player.pos.x -= 1;
			tet.player.pos.xHalf = true;
		}
	}
	if (!prevOffset.yHalf && newOffset.yHalf) { // Move half up
		if (tet.player.pos.yHalf) {
			tet.player.pos.y += 1;
			tet.player.pos.yHalf = false;
		} else {
			tet.player.pos.yHalf = true;
		}
	}
	if (prevOffset.yHalf && !newOffset.yHalf) { // Move half down
		if (tet.player.pos.yHalf) {
			tet.player.pos.yHalf = false;
		} else {
			tet.player.pos.y -= 1;
			tet.player.pos.yHalf = true;
		}
	}

	if (!tryKicks()) {
		tet.player.rotation = prevRotation;
		tet.player.pos.x = prevPos.x;
		tet.player.pos.y = prevPos.y;
		tet.player.pos.xHalf = prevPos.xHalf;
		tet.player.pos.yHalf = prevPos.yHalf;
	}
}

/**
 * Attempt to shift the player piece in the given direction.
 * @param direction -1 for left, 1 for right
 */
static void shift(int direction)
{
	assert(direction == 1 || direction == -1);

	// Remove any half-grid offset
	if (tet.player.pos.xHalf) {
		tet.player.pos.xHalf = false;
		if (direction == 1)
			tet.player.pos.x += 1;
		return;
	}

	tet.player.pos.x += direction;
	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);
	if (isOverlap(playerPiece, tet.player.pos)) {
		tet.player.pos.x -= direction;
	}
}

/**
 * Return a random new piece type using the broken7bag system.
 * @return Picked piece type
 */
static mino randomPiece(void)
{
	// Refill 7bag
	if (!darraySize(tet.player.sevenBag)) {
		for (mino m = MinoNone + 1; m < MinoGarbage; m += 1) {
			mino* new = darrayProduce(tet.player.sevenBag);
			*new = m;
		}
	}

	// Refill 6bag using 7bag
	if (!darraySize(tet.player.sixBag)) {
		size_t index = rngInt(tet.rng, darraySize(tet.player.sevenBag));
		mino chosen = *(mino*)darrayGet(tet.player.sevenBag, index);
		darrayRemove(tet.player.sevenBag, index);

		for (mino m = MinoNone + 1; m < MinoGarbage; m += 1) {
			if (m == chosen) continue;
			mino* new = darrayProduce(tet.player.sixBag);
			*new = m;
		}
	}

	// Pick random piece from 6bag
	size_t index = rngInt(tet.rng, darraySize(tet.player.sixBag));
	mino chosen = *(mino*)darrayGet(tet.player.sixBag, index);
	darrayRemove(tet.player.sixBag, index);
	return chosen;
}

/**
 * Prepare the player piece for a brand new adventure at the top of the field.
 */
static void spawnPiece(void)
{

	tet.player.state = PlayerSpawned; // Some moves restricted on first frame
	tet.player.pos.x = SpawnX;
	tet.player.pos.y = SpawnY;

	// Picking the next piece
	tet.player.type = tet.player.preview;
	tet.player.preview = randomPiece();

	tet.player.ySub = 0;
	tet.player.lockDelay = 0;
	tet.player.spawnDelay = 0;
	tet.player.clearDelay = 0;
	tet.player.rotation = SpinNone;

	HalfrsPoint offset = halfrsGetPieceOffset(tet.player.type, tet.player.rotation);
	tet.player.pos.x += offset.x;
	tet.player.pos.xHalf += offset.xHalf;
	tet.player.pos.y += offset.y;
	tet.player.pos.yHalf += offset.yHalf;

	// IRS
	if (inputHeld(InputButton2)) {
		rotate(-1);
	} else {
		if (inputHeld(InputButton1) || inputHeld(InputButton3))
			rotate(1);
	}

	// Increase gravity
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
	}
}

/**
 * Check whether the player piece could move down one cell without overlapping
 * the field.
 * @return true if no overlap, false if overlap
 */
static bool canDrop(void)
{
	// Half-grid offset guarantees that this is possible
	if (tet.player.pos.yHalf) return true;

	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);
	return !isOverlap(playerPiece, (HalfrsPoint){
		.x = tet.player.pos.x,
		.y = tet.player.pos.y - 1,
		.xHalf = tet.player.pos.xHalf,
		.yHalf = false
	});
}

/**
 * Move the player piece down one cell if possible, also calculating other
 * appropriate values.
 */
static void drop(void)
{
	if (!canDrop())
		return;

	tet.player.lockDelay = 0;
	if (tet.player.pos.yHalf)
		tet.player.pos.yHalf = 0;
	else
		tet.player.pos.y -= 1;
}

/**
 * Stamp player piece onto the grid.
 */
static void lock(void)
{
	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);

	// Need to get rid of the offset cleanly
	while (tet.player.pos.xHalf) {
		tet.player.pos.xHalf = false;

		bool leftDrop = canDrop();
		tet.player.pos.x += 1;
		bool rightDrop = canDrop();
		tet.player.pos.x -= 1;

		if (!leftDrop && rightDrop)
			continue;
		if (leftDrop && !rightDrop) {
			tet.player.pos.x += 1;
			continue;
		}
		if (!leftDrop && !rightDrop) {
			if (tet.player.lastDirection == InputRight)
				tet.player.pos.x += 1;
			continue;
		}
		logDebug(applog, "Piece being locked in midair - not supposed to happen!");
	}

	fieldStampPiece(tet.field, playerPiece, *(point2i*)&tet.player.pos, tet.player.type);
	tet.player.state = PlayerSpawn;
	easeRestart(&lockFlash);
}

void halfrsInit(void)
{
	if (initialized) return;

	// Logic init
	structClear(tet);
	tet.frame = -1;
	tet.ready = 3 * 50;
	tet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	tet.rng = rngCreate((uint64_t)time(null));
	tet.player.autoshiftDelay = AutoshiftRepeat; // Starts out pre-charged
	tet.player.spawnDelay = SpawnDelay; // Start instantly
	tet.player.gravity = 3;

	tet.player.sixBag = darrayCreate(sizeof(mino));
	tet.player.sevenBag = darrayCreate(sizeof(mino));
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
	logDebug(applog, u8"Halfrs sublayer initialized");
}

void halfrsCleanup(void)
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
	darrayDestroy(tet.player.sevenBag);
	tet.player.sevenBag = null;
	darrayDestroy(tet.player.sixBag);
	tet.player.sixBag = null;
	rngDestroy(tet.rng);
	tet.rng = null;
	fieldDestroy(tet.field);
	tet.field = null;
	initialized = false;
	logDebug(applog, u8"Halfrs sublayer cleaned up");
}

/**
 * Populate and rotate the input arrays for press and hold detection.
 * @param inputs List of this frame's new inputs
 */
static void halfrsUpdateInputs(darray* inputs)
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
static void halfrsUpdateState(void)
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
static void halfrsUpdateRotation(void)
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
static void halfrsUpdateShift(void)
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
static void halfrsUpdateClear(void)
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
static void halfrsUpdateSpawn(void)
{
	if (tet.state != TetrionPlaying)
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
static void halfrsUpdateGravity(void)
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
static void halfrsUpdateLocking(void)
{
	if (tet.player.state != PlayerActive || tet.state != TetrionPlaying)
		return;
	if (canDrop())
		return;

	tet.player.lockDelay += 1;
	// Two sources of locking: lock delay expired, manlock
	if (tet.player.lockDelay > LockDelay || inputHeld(InputDown))
		lock();
}

void halfrsAdvance(darray* inputs)
{
	assert(inputs);
	assert(initialized);

	halfrsUpdateInputs(inputs);
	halfrsUpdateState();
	halfrsUpdateRotation();
	halfrsUpdateShift();
	halfrsUpdateClear();
	halfrsUpdateSpawn();
	halfrsUpdateGravity();
	halfrsUpdateLocking();
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Draw the scene model, which visually wraps the tetrion field.
 */
static void halfrsDrawScene(void)
{
	float boost = easeApply(&comboFade);
	modelDraw(scene, 1, (color4[]){{boost, boost, boost, 1.0f}}, null, &IdentityMatrix);
}

/**
 * Draw the guide model, helping a beginner player keep track of columns.
 */
static void halfrsDrawGuide(void)
{
	modelDraw(guide, 1, (color4[]){Color4White}, null, &IdentityMatrix);
}

/**
 * Queue the contents of the tetrion field.
 */
static void halfrsQueueField(void)
{
	// A bit out of place here, but no need to get this more than once
	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);

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
			color4Copy(*highlight, ((color4){LockFlashBrightness, LockFlashBrightness, LockFlashBrightness, flash}));
		} else {
			color4Copy(*highlight, Color4Clear);
		}

		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
	}
}

/**
 * Queue the player piece on top of the field.
 */
static void halfrsQueuePlayer(void)
{
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + tet.player.pos.x;
		float y = (*playerPiece)[i].y + tet.player.pos.y;
		x += tet.player.pos.xHalf? 0.5f : 0.0f;
		y += tet.player.pos.yHalf? 0.5f : 0.0f;

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
			lockDim.start -= tet.player.lockDelay * HalfrsUpdateTick;
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
static void halfrsQueueGhost(void)
{
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = halfrsGetPiece(tet.player.type, tet.player.rotation);
	HalfrsPoint ghostPos = tet.player.pos;
	ghostPos.yHalf = false;
	while (!isOverlap(playerPiece, ghostPos))
		ghostPos.y -= 1; // Drop down as much as possible
	ghostPos.y += 1; // Revert the last failure

	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + ghostPos.x;
		float y = (*playerPiece)[i].y + ghostPos.y;
		x += tet.player.pos.xHalf? 0.5f : 0.0f;

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
static void halfrsQueuePreview(void)
{
	if (tet.player.preview == MinoNone)
		return;
	piece* previewPiece = halfrsGetPiece(tet.player.preview, SpinNone);
	HalfrsPoint previewOffset = halfrsGetPieceOffset(tet.player.preview, SpinNone);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*previewPiece)[i].x + PreviewX + previewOffset.x;
		float y = (*previewPiece)[i].y + PreviewY + previewOffset.y;
		x += previewOffset.xHalf? 0.5f : 0.0f;
		y += previewOffset.yHalf? 0.5f : 0.0f;

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
static void halfrsDrawQueuedBlocks(void)
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

static void halfrsBorderQueue(point3f pos, size3f size, color4 color)
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
static void halfrsDrawBorder(void)
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
			halfrsBorderQueue((point3f){tx, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Right
		if (!fieldGet(tet.field, (point2i){x + 1, y}))
			halfrsBorderQueue((point3f){tx + 0.875f, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down
		if (!fieldGet(tet.field, (point2i){x, y - 1}))
			halfrsBorderQueue((point3f){tx + 0.125f, ty, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up
		if (!fieldGet(tet.field, (point2i){x, y + 1}))
			halfrsBorderQueue((point3f){tx + 0.125f, ty + 0.875f, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Left
		if (!fieldGet(tet.field, (point2i){x - 1, y - 1})
			|| !fieldGet(tet.field, (point2i){x - 1, y})
			|| !fieldGet(tet.field, (point2i){x, y - 1}))
			halfrsBorderQueue((point3f){tx, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Right
		if (!fieldGet(tet.field, (point2i){x + 1, y - 1})
			|| !fieldGet(tet.field, (point2i){x + 1, y})
			|| !fieldGet(tet.field, (point2i){x, y - 1}))
			halfrsBorderQueue((point3f){tx + 0.875f, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Left
		if (!fieldGet(tet.field, (point2i){x - 1, y + 1})
			|| !fieldGet(tet.field, (point2i){x - 1, y})
			|| !fieldGet(tet.field, (point2i){x, y + 1}))
			halfrsBorderQueue((point3f){tx, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Right
		if (!fieldGet(tet.field, (point2i){x + 1, y + 1})
			|| !fieldGet(tet.field, (point2i){x + 1, y})
			|| !fieldGet(tet.field, (point2i){x, y + 1}))
			halfrsBorderQueue((point3f){tx + 0.875f, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
	}

	modelDraw(border, darraySize(borderTransforms),
		darrayData(borderTints), null,
		darrayData(borderTransforms));
	darrayClear(borderTints);
	darrayClear(borderTransforms);
}

void halfrsDraw(void)
{
	assert(initialized);

	glClearColor(0.010f, 0.276f, 0.685f, 1.0f); //TODO make into layer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	worldSetAmbientColor((color3){0.010f, 0.276f, 0.685f});
	halfrsDrawScene();
	halfrsDrawGuide();
	halfrsQueueField();
	halfrsQueuePlayer();
	halfrsQueueGhost();
	halfrsQueuePreview();
	halfrsDrawQueuedBlocks();
	halfrsDrawBorder();
}
