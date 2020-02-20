/**
 * Implementation of pure.h
 * @file
 */

#include "pure.h"

#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "puretables.h"
#include "renderer.h"
#include "mapper.h"
#include "mino.h"
#include "util.h"
#include "log.h"

#define FieldWidth 10u ///< Width of #field
#define FieldHeight 22u ///< Height of #field

#define SpawnX 3 ///< X position of player piece spawn
#define SpawnY 18 ///< Y position of player piece spawn
#define SubGrid 256 ///< Number of subpixels per cell, used for gravity

#define HistorySize 4 ///< Number of past pieces to remember for rng bias
#define MaxRerolls 4 ///< Number of times the rng avoids pieces from history

#define ClockFrequency 60.0 ///< Rate of the ingame timer (slightly off-sync)
#define ClockTick (secToNsec(1) / ClockFrequency) ///< Duration of a timer tick

#define SoftDrop 256 ///< Speed of gravity while holding down, in subgrids
#define AutoshiftCharge 16 ///< Frames direction has to be held before autoshift
#define AutoshiftRepeat 1 ///< Frames between autoshifts
#define LockDelay 30 ///< Frames a piece can spend on the stack before locking
#define ClearOffset 4 ///< Frames between piece lock and line clear
#define ClearDelay 41 ///< Frames between line clear and thump
#define SpawnDelay 30 ///< Frames between lock/thumb and new piece spawn

#define FieldHeightVisible 20u ///< Number of bottom rows the player can see
#define PreviewX -2.0f ///< X offset of preview piece
#define PreviewY 21.0f ///< Y offset of preview piece
#define FieldDim 0.4f ///< Multiplier of field block color
#define ExtraRowDim 0.25f ///< Multiplier of field block alpha above the scene
#define GhostDim 0.2f ///< Multiplier of ghost block alpha
#define BorderDim 0.5f ///< Multiplier of border alpha

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
	InputType lastDirection; ///< None, Left or Right - used to improve kb play

	PlayerState state;
	mino type; ///< Current player piece
	mino preview; ///< Next player piece
	mino history[HistorySize]; ///< Past player pieces
	spin rotation; ///< ::spin of current piece
	point2i pos; ///< Position of current piece
	int ySub; ///< Y subgrid of current piece

	int autoshiftDirection; ///< Autoshift state: -1 left, 1 right, 0 none
	int autoshiftCharge;
	int autoshiftDelay;
	int lockDelay;
	int clearDelay;
	int spawnDelay;

	int level;
	int dropBonus; ///< Accumulated soft drop bonus score
} Player;

/// State of tetrion FSM
typedef enum TetrionState {
	TetrionNone, ///< zero value
	TetrionReady, ///< intro
	TetrionPlaying, ///< gameplay
	TetrionOutro, ///< outro
	TetrionSize ///< terminator
} TetrionState;

/// State of a grade requirement
typedef enum ReqStatus {
	ReqNone, ///< not tested yet
	ReqPassed, ///< success
	ReqFailed, ///< failure
	ReqSize ///< terminator
} ReqStatus;

/// A play's logical state
typedef struct Tetrion {
	TetrionState state;
	int ready; ///< Countdown timer
	int frame; ///< Frame counter since #ready = 0

	Field* field;
	bool linesCleared[FieldHeight]; ///< Storage for line clears pending a thump
	Player player;
	Rng* rng;

	int score;
	int combo; ///< Holdover combo from previous piece
	int grade;
	ReqStatus reqs[countof(PureRequirements)]; ///< Max grade requirements
} Tetrion;

/// Full state of the mode
static Tetrion tet = {0};

static Model* scene = null;

static Model* block = null;
static darray* blockTintsOpaque = null;
static darray* blockTransformsOpaque = null;
static darray* blockTintsAlpha = null;
static darray* blockTransformsAlpha = null;

static Model* border = null;
static darray* borderTints = null;
static darray* borderTransforms = null;

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
 * Try to kick the player piece into a legal position.
 * @return true if player piece already legal or successfully kicked, false if
 * no kick was possible
 */
static bool tryKicks(void)
{
	static int preference = 1;
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // Original position

	if (tet.player.state == PlayerSpawned)
		return false; // If this is IRS, don't attempt kicks
	if (tet.player.type == MinoI)
		return false; // I doesn't kick

	// The annoying special treatment of LTJ middle column
	if (tet.player.rotation % 2 == 1 && // Vertical orientation only
		(tet.player.type == MinoL ||
			tet.player.type == MinoT ||
			tet.player.type == MinoJ)) {
		for (size_t i = 0; i < MinosPerPiece; i += 1) {
			point2i fieldPos = {
				.x = tet.player.pos.x + (*playerPiece)[i].x,
				.y = tet.player.pos.y + (*playerPiece)[i].y
			};
			if (fieldGet(tet.field, fieldPos)) {
				if ((*playerPiece)[i].x == CenterColumn)
					return false;
				else // We hit the exception to the middle column rule
					break;
			}
		}
	}

	// Now that every exception is filtered out, we can actually do it
	tet.player.pos.x += preference;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the right
	tet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		return true; // 1 to the left
	tet.player.pos.x += preference;
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
	if (direction == 1)
		spinClockwise(&tet.player.rotation);
	else
		spinCounterClockwise(&tet.player.rotation);
	if (!tryKicks())
		tet.player.rotation = prevRotation;
}

/**
 * Attempt to shift the player piece in the given direction.
 * @param direction -1 for left, 1 for right
 */
static void shift(int direction)
{
	assert(direction == 1 || direction == -1);
	tet.player.pos.x += direction;
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	if (pieceOverlapsField(playerPiece, tet.player.pos, tet.field)) {
		tet.player.pos.x -= direction;
	}
}

/**
 * Return a random new piece type, taking into account history bias and other
 * restrictions.
 * @return Picked piece type
 */
static mino randomPiece(void)
{
	bool first = false;
	if (tet.player.history[0] == MinoNone) { // History empty, initialize
		first = true;
		for (int i = 0; i < HistorySize; i += 1)
			tet.player.history[i] = MinoZ;
	}

	mino result = MinoNone;
	for (int i = 0; i < MaxRerolls; i += 1) {
		result = rngInt(tet.rng, MinoGarbage - 1) + 1;
		while (first && // Unfair first piece prevention
			(result == MinoS ||
				result == MinoZ ||
				result == MinoO))
			result = rngInt(tet.rng, MinoGarbage - 1) + 1;

		// If piece in history, reroll
		bool valid = true;
		for (int j = 0; j < HistorySize; j += 1) {
			if (result == tet.player.history[j])
				valid = false;
		}
		if (valid)
			break;
	}

	// Rotate history
	for (int i = HistorySize - 2; i >= 0; i -= 1) {
		tet.player.history[i + 1] = tet.player.history[i];
	}
	tet.player.history[0] = result;
	return result;
}

/**
 * Return the next upcoming levelstop for a given level.
 * @param level
 * @return Next levelstop value, or the final levelstop
 */
static int getLevelstop(int level)
{
	int result = (level / 100 + 1) * 100;
	if (result >= 1000)
		result = 999;
	return result;
}

/**
 * Increase the level counter after a clear or piece spawn.
 * @param count Number of levels to add
 * @param strong true to break past levelstop, false to get stopped
 */
static void addLevels(int count, bool strong)
{
	int levelstop = getLevelstop(tet.player.level);
	tet.player.level += count;
	if (!strong && tet.player.level >= levelstop)
		tet.player.level = levelstop - 1;
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
	tet.player.dropBonus = 0;

	// IRS
	if (inputHeld(InputButton2)) {
		rotate(-1);
	} else {
		if (inputHeld(InputButton1) || inputHeld(InputButton3))
			rotate(1);
	}

	addLevels(1, false);

	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	if (pieceOverlapsField(playerPiece, tet.player.pos, tet.field))
		gameOver();
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
		fieldClearRow(tet.field, y);
	}
	return count;
}

/**
 * Award score for a line clear. Takes into account state and various bonuses.
 * @param lines Number of lines cleared
 */
static void addScore(int lines)
{
	int score;
	score = tet.player.level + lines;
	int remainder = score % 4;
	score /= 4;
	if (remainder)
		score += 1;
	score += tet.player.dropBonus;
	score *= lines;
	tet.combo += 2 * lines - 2;
	score *= tet.combo;
	if (fieldIsEmpty(tet.field))
		score *= 4; // Bravo bonus

	tet.score += score;
}

/**
 * Return time value of the visible clock. This is intentionally out of sync
 * with real playtime.
 * @param frame Frames since ready = 0
 * @return Time since ready = 0
 */
static nsec getClock(int frame)
{
	return frame * ClockTick;
}

/**
 * Check all requirements and update their status.
 */
static void updateRequirements(void)
{
	assert(countof(tet.reqs) == countof(PureRequirements));
	for (size_t i = 0; i < countof(tet.reqs); i += 1) {
		if (tet.reqs[i])
			continue; // Only check each treshold once, when reached
		if (tet.player.level < PureRequirements[i].level)
			return; // PureThreshold not reached yet
		if (tet.score >= PureRequirements[i].score &&
			getClock(tet.frame) <= PureRequirements[i].time)
			tet.reqs[i] = ReqPassed;
		else
			tet.reqs[i] = ReqFailed;
	}
}

/**
 * Check whether player is qualified to obtain max grade.
 * @return true if qualified, flase if at least one requirement is failed
 * or not tested yet
 */
static bool requirementsMet(void)
{
	for (size_t i = 0; i < countof(tet.reqs); i += 1) {
		if (tet.reqs[i] != ReqPassed)
			return false;
	}
	return true;
}

/**
 * Set grade to the highest one the player is qualified for.
 */
static void updateGrade(void)
{
	for (size_t i = 0; i < countof(PureGrades); i += 1) {
		if (tet.score < PureGrades[i])
			return;
		if (i == countof(PureGrades) - 1 &&
			(!requirementsMet() || tet.player.level < 999))
			return; // Final grade, requirements not met
		tet.grade = i;
	}
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
 * Return the gravity that applies at a specific level.
 * @param level
 * @return Gravity speed in subgrids
 */
static int getGravity(int level)
{
	int result = 0;
	for (int i = 0; i < countof(PureThresholds); i += 1) {
		if (level < PureThresholds[i].level)
			break;
		result = PureThresholds[i].gravity;
	}
	return result;
}

/**
 * Check whether the player piece could move down one cell without overlapping
 * the field.
 * @return true if no overlap, false if overlap
 */
static bool canDrop(void)
{
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
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

	tet.player.lockDelay = 0;
	tet.player.pos.y -= 1;
	if (inputHeld(InputDown))
		tet.player.dropBonus += 1;
}

/**
 * Stamp player piece onto the grid.
 */
static void lock(void)
{
	if (inputHeld(InputDown))
		tet.player.dropBonus += 1; // Lock frame can also increase this
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	fieldStampPiece(tet.field, playerPiece, tet.player.pos, tet.player.type);
	tet.player.state = PlayerSpawn;
}

void pureInit(void)
{
	if (initialized) return;

	// Logic init
	structClear(tet);
	tet.combo = 1;
	tet.frame = -1;
	tet.ready = 3 * 50;
	tet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	tet.rng = rngCreate((uint64_t)time(null));
	tet.player.level = -1;
	tet.player.autoshiftDelay = AutoshiftRepeat; // Starts out pre-charged
	tet.player.spawnDelay = SpawnDelay; // Start instantly
	tet.player.preview = randomPiece();

	tet.state = TetrionReady;

	// Render init
	scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	block = modelCreatePhong(u8"block",
#include "meshes/block.mesh"
	);
	blockTintsOpaque = darrayCreate(sizeof(color4));
	blockTransformsOpaque = darrayCreate(sizeof(mat4x4));
	blockTintsAlpha = darrayCreate(sizeof(color4));
	blockTransformsAlpha = darrayCreate(sizeof(mat4x4));
	border = modelCreateFlat(u8"border",
#include "meshes/border.mesh"
	);
	borderTints = darrayCreate(sizeof(color4));
	borderTransforms = darrayCreate(sizeof(mat4x4));

	initialized = true;
	logDebug(applog, u8"Pure sublayer initialized");
}

void pureCleanup(void)
{
	if (!initialized) return;
	if (borderTransforms) {
		darrayDestroy(borderTransforms);
		borderTransforms = null;
	}
	if (borderTints) {
		darrayDestroy(borderTints);
		borderTints = null;
	}
	if (border) {
		modelDestroy(border);
		border = null;
	}
	if (blockTransformsAlpha) {
		darrayDestroy(blockTransformsAlpha);
		blockTransformsAlpha = null;
	}
	if (blockTintsAlpha) {
		darrayDestroy(blockTintsAlpha);
		blockTintsAlpha = null;
	}
	if (blockTransformsOpaque) {
		darrayDestroy(blockTransformsOpaque);
		blockTransformsOpaque = null;
	}
	if (blockTintsOpaque) {
		darrayDestroy(blockTintsOpaque);
		blockTintsOpaque = null;
	}
	if (block) {
		modelDestroy(block);
		block = null;
	}
	if (scene) {
		modelDestroy(scene);
		scene = null;
	}
	if (tet.rng) {
		rngDestroy(tet.rng);
		tet.rng = null;
	}
	if (tet.field) {
		fieldDestroy(tet.field);
		tet.field = null;
	}
	initialized = false;
	logDebug(applog, u8"Pure sublayer cleaned up");
}

/**
 * Populate and rotate the input arrays for press and hold detection.
 * @param inputs List of this frame's new inputs
 */
static void pureUpdateInputs(darray* inputs)
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
static void pureUpdateState(void)
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
static void pureUpdateRotation(void)
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
static void pureUpdateShift(void)
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
static void pureUpdateClear(void)
{
	// Line clear check is delayed by the clear offset
	if (tet.player.state == PlayerSpawn &&
		tet.player.spawnDelay + 1 == ClearOffset) {
		int clearedCount = checkClears();
		if (clearedCount) {
			tet.player.state = PlayerClear;
			tet.player.clearDelay = 0;
			addScore(clearedCount);
			addLevels(clearedCount, true);
			updateRequirements();
			updateGrade();
		} else { // Piece locked without a clear
			tet.combo = 1;
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
static void pureUpdateSpawn(void)
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
static void pureUpdateGravity(void)
{
	if (tet.state == TetrionOutro)
		return; // Prevent zombie blocks
	if (tet.player.state != PlayerSpawned && tet.player.state != PlayerActive)
		return;

	int gravity = getGravity(tet.player.level);
	if (tet.player.state == PlayerActive) {
		if (inputHeld(InputDown) && gravity < SoftDrop)
			gravity = SoftDrop;
	}

	if (canDrop()) // Queue up the gravity drops
		tet.player.ySub += gravity;
	else
		tet.player.ySub = 0;

	while (tet.player.ySub >= SubGrid) { // Drop until queue empty
		drop();
		tet.player.ySub -= SubGrid;
	}
}

/**
 * Lock player piece by lock delay expiry or manual lock.
 */
static void pureUpdateLocking(void)
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

/**
 * Win the game. Try to get this function called while playing.
 */
static void pureUpdateWin(void)
{
	if (tet.player.level >= 999)
		gameOver();
}

void pureAdvance(darray* inputs)
{
	assert(inputs);
	assert(initialized);

	pureUpdateInputs(inputs);
	pureUpdateState();
	pureUpdateRotation();
	pureUpdateShift();
	pureUpdateClear();
	pureUpdateSpawn();
	pureUpdateGravity();
	pureUpdateLocking();
	pureUpdateWin();
}

/**
 * Draw the scene model, which visually wraps the tetrion field.
 */
static void pureDrawScene(void)
{
	modelDraw(scene, 1, (color4[]){Color4White}, &IdentityMatrix);
}

/**
 * Queue the contents of the tetrion field.
 */
static void pureQueueField(void)
{
	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;
		mino type = fieldGet(tet.field, (point2i){x, y});
		if (type == MinoNone) continue;

		color4* tint = null;
		mat4x4* transform = null;
		if (minoColor(type).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(type));
		tint->r *= FieldDim;
		tint->g *= FieldDim;
		tint->b *= FieldDim;
		if (y >= FieldHeightVisible)
			tint->a *= ExtraRowDim;
		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
	}
}

/**
 * Queue the player piece on top of the field.
 */
static void pureQueuePlayer(void)
{
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*playerPiece)[i].x + tet.player.pos.x;
		float y = (*playerPiece)[i].y + tet.player.pos.y;

		color4* tint = null;
		mat4x4* transform = null;
		if (minoColor(tet.player.type).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(tet.player.type));
		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
	}
}

/**
 * Queue the ghost piece, if it should be visible.
 */
static void pureQueueGhost(void)
{
	if (tet.player.level >= 100) return;
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;

	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
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
		mat4x4* transform = darrayProduce(blockTransformsAlpha);

		color4Copy(*tint, minoColor(tet.player.type));
		tint->a *= GhostDim;
		mat4x4_translate(*transform, x - (signed)(FieldWidth / 2), y, 0.0f);
	}
}

/**
 * Queue the preview piece on top of the field.
 */
static void pureQueuePreview(void)
{
	if (tet.player.preview == MinoNone)
		return;
	piece* previewPiece = getPiece(tet.player.preview, SpinNone);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		float x = (*previewPiece)[i].x + PreviewX;
		float y = (*previewPiece)[i].y + PreviewY;
		if (tet.player.preview == MinoI)
			y -= 1;

		color4* tint = null;
		mat4x4* transform = null;
		if (minoColor(tet.player.preview).a == 1.0) {
			tint = darrayProduce(blockTintsOpaque);
			transform = darrayProduce(blockTransformsOpaque);
		} else {
			tint = darrayProduce(blockTintsAlpha);
			transform = darrayProduce(blockTransformsAlpha);
		}

		color4Copy(*tint, minoColor(tet.player.preview));
		mat4x4_translate(*transform, x, y, 0.0f);
	}
}

/**
 * Draw all queued blocks with alpha pre-pass.
 */
static void pureDrawQueuedBlocks(void)
{
	modelDraw(block, darraySize(blockTransformsOpaque),
		darrayData(blockTintsOpaque),
		darrayData(blockTransformsOpaque));
	darrayClear(blockTintsOpaque);
	darrayClear(blockTransformsOpaque);
	rendererDepthOnlyBegin();
	modelDraw(block, darraySize(blockTransformsAlpha),
		darrayData(blockTintsAlpha),
		darrayData(blockTransformsAlpha));
	rendererDepthOnlyEnd();
	modelDraw(block, darraySize(blockTransformsAlpha),
		darrayData(blockTintsAlpha),
		darrayData(blockTransformsAlpha));
	darrayClear(blockTintsAlpha);
	darrayClear(blockTransformsAlpha);
}

static void borderQueue(point3f pos, size3f size, color4 color)
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
static void pureDrawBorder(void)
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
			borderQueue((point3f){tx, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Right
		if (!fieldGet(tet.field, (point2i){x + 1, y}))
			borderQueue((point3f){tx + 0.875f, ty + 0.125f, 0.0f},
				(size3f){0.125f, 0.75f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down
		if (!fieldGet(tet.field, (point2i){x, y - 1}))
			borderQueue((point3f){tx + 0.125f, ty, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up
		if (!fieldGet(tet.field, (point2i){x, y + 1}))
			borderQueue((point3f){tx + 0.125f, ty + 0.875f, 0.0f},
				(size3f){0.75f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Left
		if (!fieldGet(tet.field, (point2i){x - 1, y - 1})
			|| !fieldGet(tet.field, (point2i){x - 1, y})
			|| !fieldGet(tet.field, (point2i){x, y - 1}))
			borderQueue((point3f){tx, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Down Right
		if (!fieldGet(tet.field, (point2i){x + 1, y - 1})
			|| !fieldGet(tet.field, (point2i){x + 1, y})
			|| !fieldGet(tet.field, (point2i){x, y - 1}))
			borderQueue((point3f){tx + 0.875f, ty, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Left
		if (!fieldGet(tet.field, (point2i){x - 1, y + 1})
			|| !fieldGet(tet.field, (point2i){x - 1, y})
			|| !fieldGet(tet.field, (point2i){x, y + 1}))
			borderQueue((point3f){tx, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
		// Up Right
		if (!fieldGet(tet.field, (point2i){x + 1, y + 1})
			|| !fieldGet(tet.field, (point2i){x + 1, y})
			|| !fieldGet(tet.field, (point2i){x, y + 1}))
			borderQueue((point3f){tx + 0.875f, ty + 0.875f, 0.0f},
				(size3f){0.125f, 0.125f, 1.0f},
				(color4){1.0f, 1.0f, 1.0f, alpha});
	}

	modelDraw(border, darraySize(borderTransforms),
		darrayData(borderTints),
		darrayData(borderTransforms));
	darrayClear(borderTints);
	darrayClear(borderTransforms);
}

void pureDraw(void)
{
	assert(initialized);

	rendererClear((color3){0.010f, 0.276f, 0.685f}); //TODO make into layer
	pureDrawScene();
	pureQueueField();
	pureQueuePlayer();
	pureQueueGhost();
	pureQueuePreview();
	pureDrawQueuedBlocks();
	pureDrawBorder();
}
