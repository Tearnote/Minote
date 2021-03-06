/**
 * Implementation of mrs.h
 * @file
 */

#include "mrs.hpp"

#include <stdint.h>
#include <time.h>
#include "mrsdef.hpp"
#include "base/log.hpp"

using namespace minote;

int mrsDebugPauseSpawn = 0;
int mrsDebugInfLock = 0;

static bool initialized = false;

Tetrion mrsTet = {};

/**
 * Test whether an input has been pressed just now.
 * @param type ::InputType to test
 * @return true if just pressed, false otherwise
 */
#define inputPressed(type) \
    (mrsTet.player.actionMap[(+type)] && !mrsTet.player.actionMapPrev[(+type)])

/**
 * Test whether an input is pressed during this frame.
 * @param type ::InputType to test
 * @return true if pressed, false if not pressed
 */
#define inputHeld(type) \
    (mrsTet.player.actionMap[(+type)])

static void updateShape(void)
{
	arrayCopy(mrsTet.player.shape, MrsPieces[mrsTet.player.type]);
	pieceRotate(mrsTet.player.shape, mrsTet.player.rotation);
}

/**
 * Try to kick the player piece into a legal position.
 * @param prevRotation piece rotation prior to the kick
 * @return true if player piece already legal or successfully kicked, false if
 * no kick was possible
 */
static bool tryKicks(spin prevRotation)
{
	if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
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
		if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
			return true; // 1 to the right
		mrsTet.player.pos.y -= 1;
	}

	// Now that every exception is filtered out, we can try the default kicks
	int preference = mrsTet.player.lastDirection == Action::Type::Right ? 1 : -1;

	// Down
	mrsTet.player.pos.y -= 1;
	if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the right
	mrsTet.player.pos.y += 1;

	// Left/right
	mrsTet.player.pos.x += preference;
	if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the right
	mrsTet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the left
	mrsTet.player.pos.x += preference;

	// Down+left/right
	mrsTet.player.pos.y -= 1;
	mrsTet.player.pos.x += preference;
	if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
		return true; // 1 to the right
	mrsTet.player.pos.x -= preference * 2;
	if (!pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
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
	ASSERT(direction == 1 || direction == -1);
	spin prevRotation = mrsTet.player.rotation;
	ivec2 prevPosition = mrsTet.player.pos;

	if (direction == 1)
		spinClockwise(&mrsTet.player.rotation);
	else
		spinCounterClockwise(&mrsTet.player.rotation);
	updateShape();

	// Apply crawl offsets to I
	if (mrsTet.player.type == MinoI) {
		if (prevRotation == SpinNone && mrsTet.player.rotation == Spin90)
			mrsTet.player.pos.y -= 1;
		if (prevRotation == Spin90 && mrsTet.player.rotation == Spin180)
			mrsTet.player.pos.y += 1;
		if (prevRotation == Spin180 && mrsTet.player.rotation == Spin270)
			mrsTet.player.pos.x -= 1;
		if (prevRotation == Spin270 && mrsTet.player.rotation == SpinNone)
			mrsTet.player.pos.x -= 1;

		if (prevRotation == SpinNone && mrsTet.player.rotation == Spin270)
			mrsTet.player.pos.x += 1;
		if (prevRotation == Spin270 && mrsTet.player.rotation == Spin180)
			mrsTet.player.pos.x += 1;
		if (prevRotation == Spin180 && mrsTet.player.rotation == Spin90)
			mrsTet.player.pos.y -= 1;
		if (prevRotation == Spin90 && mrsTet.player.rotation == SpinNone)
			mrsTet.player.pos.y += 1;
	}

	// Apply crawl offsets to S and Z
	if (mrsTet.player.type == MinoS || mrsTet.player.type == MinoZ) {
		if (prevRotation == SpinNone && mrsTet.player.rotation == Spin90)
			mrsTet.player.pos.x -= 1;
		if (prevRotation == Spin90 && mrsTet.player.rotation == Spin180)
			mrsTet.player.pos.y -= 1;
		if (prevRotation == Spin180 && mrsTet.player.rotation == Spin270)
			mrsTet.player.pos.y += 1;
		if (prevRotation == Spin270 && mrsTet.player.rotation == SpinNone)
			mrsTet.player.pos.x -= 1;

		if (prevRotation == SpinNone && mrsTet.player.rotation == Spin270)
			mrsTet.player.pos.x += 1;
		if (prevRotation == Spin270 && mrsTet.player.rotation == Spin180)
			mrsTet.player.pos.y -= 1;
		if (prevRotation == Spin180 && mrsTet.player.rotation == Spin90)
			mrsTet.player.pos.y += 1;
		if (prevRotation == Spin90 && mrsTet.player.rotation == SpinNone)
			mrsTet.player.pos.x += 1;
	}

	// Keep O in place
	if (mrsTet.player.type == MinoO) {
		if (prevRotation == SpinNone && mrsTet.player.rotation == Spin90)
			mrsTet.player.pos.y -= 1;
		if (prevRotation == Spin90 && mrsTet.player.rotation == Spin180)
			mrsTet.player.pos.x += 1;
		if (prevRotation == Spin180 && mrsTet.player.rotation == Spin270)
			mrsTet.player.pos.y += 1;
		if (prevRotation == Spin270 && mrsTet.player.rotation == SpinNone)
			mrsTet.player.pos.x -= 1;

		if (prevRotation == SpinNone && mrsTet.player.rotation == Spin270)
			mrsTet.player.pos.x += 1;
		if (prevRotation == Spin270 && mrsTet.player.rotation == Spin180)
			mrsTet.player.pos.y -= 1;
		if (prevRotation == Spin180 && mrsTet.player.rotation == Spin90)
			mrsTet.player.pos.x -= 1;
		if (prevRotation == Spin90 && mrsTet.player.rotation == SpinNone)
			mrsTet.player.pos.y += 1;
	}

	if (!tryKicks(prevRotation)) {
		mrsTet.player.rotation = prevRotation;
		mrsTet.player.pos.x = prevPosition.x;
		mrsTet.player.pos.y = prevPosition.y;
		updateShape();
	}
}

/**
 * Attempt to shift the player piece in the given direction.
 * @param direction -1 for left, 1 for right
 */
static void shift(int direction)
{
	ASSERT(direction == 1 || direction == -1);
	mrsTet.player.pos.x += direction;
	if (pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field)) {
		mrsTet.player.pos.x -= direction;
	} else {
		mrsTet.player.pos.x -= direction;
		mrsEffectSlide(direction,
			(mrsTet.player.autoshiftCharge == MrsAutoshiftCharge));
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
	for (size_t i = 0; i < MinoGarbage - 1; i += 1)
		if (mrsTet.player.tokens[i] > 0) tokenTotal += mrsTet.player.tokens[i];
	ASSERT(tokenTotal);

	// Create and fill the token list
	int tokenList[tokenTotal];
	size_t tokenListIndex = 0;
	for (size_t i = 0; i < MinoGarbage - 1; i += 1) {
		if (mrsTet.player.tokens[i] <= 0) continue;
		for (size_t j = 0; j < mrsTet.player.tokens[i]; j += 1) {
			ASSERT(tokenListIndex < tokenTotal);
			tokenList[tokenListIndex] = i;
			tokenListIndex += 1;
		}
	}
	ASSERT(tokenListIndex == tokenTotal);

	// Pick a random token from the list and update the token distribution
	int picked = tokenList[mrsTet.rng.randInt(tokenTotal)];
	for (size_t i = 0; i < MinoGarbage - 1; i += 1) {
		if (i == picked)
			mrsTet.player.tokens[i] -= MinoGarbage - 1 - 1;
		else
			mrsTet.player.tokens[i] += 1;
	}

	return static_cast<mino>(picked + MinoNone + 1);
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
	mrsTet.player.pos.x = MrsSpawnX;
	mrsTet.player.pos.y = MrsSpawnY;
	mrsTet.player.yLowest = mrsTet.player.pos.y;

	// Picking the next piece
	mrsTet.player.type = mrsTet.player.preview;
	mrsTet.player.preview = randomPiece();

	mrsTet.player.ySub = 0;
	mrsTet.player.lockDelay = 0;
	mrsTet.player.spawnDelay = 0;
	mrsTet.player.clearDelay = 0;
	mrsTet.player.rotation = SpinNone;

	updateShape();

	// IRS
	if (inputHeld(Action::Type::RotCW)) {
		rotate(1);
	} else {
		if (inputHeld(Action::Type::RotCCW) || inputHeld(Action::Type::RotCCW2))
			rotate(-1);
	}

	if (pieceOverlapsField(&mrsTet.player.shape, mrsTet.player.pos, mrsTet.field))
		gameOver();

	// Increase gravity
	if (mrsTet.player.gravity < 20 * MrsSubGrid) {
		int level = mrsTet.player.gravity / 64 + 1;
		mrsTet.player.gravity += level;
	}

	mrsEffectSpawn();
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
		mrsEffectClear(y, count);
		fieldClearRow(mrsTet.field, y);
	}

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
		mrsEffectThump(y);
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
	return !pieceOverlapsField(&mrsTet.player.shape, (ivec2){
		mrsTet.player.pos.x,
		mrsTet.player.pos.y - 1
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

	// Reduce lock delay if piece dropped lower than ever
	if (mrsTet.player.pos.y < mrsTet.player.yLowest) {
		mrsTet.player.lockDelay /= 2;
		mrsTet.player.yLowest = mrsTet.player.pos.y;
	}

	if (!canDrop()) {
		int direction = 0;
		if (inputHeld(Action::Type::Left))
			direction = -1;
		else if (inputHeld(Action::Type::Right))
			direction = 1;
		mrsEffectLand(direction);
	}
}

/**
 * Stamp player piece onto the grid.
 */
static void lock(void)
{
	fieldStampPiece(mrsTet.field, &mrsTet.player.shape, mrsTet.player.pos,
		mrsTet.player.type);
	mrsTet.player.state = PlayerSpawn;
	mrsEffectLock();
}

void mrsInit(void)
{
	if (initialized) return;

	// Logic init
	mrsTet = {};
	mrsTet.frame = -1;
	mrsTet.ready = 3 * 50;
	mrsTet.field = fieldCreate((ivec2){FieldWidth, FieldHeight});
	mrsTet.player.autoshiftDelay = MrsAutoshiftRepeat; // Starts out pre-charged
	mrsTet.player.spawnDelay = MrsSpawnDelay; // Start instantly
	mrsTet.player.gravity = 3;

	mrsTet.rng.seed((uint64_t)time(nullptr));
	for (size_t i = 0; i < MinoGarbage - 1; i += 1)
		mrsTet.player.tokens[i] = MrsStartingTokens;
	do {
		mrsTet.player.preview = randomPiece();
	}
	while (mrsTet.player.preview == MinoO
		|| mrsTet.player.preview == MinoS
		|| mrsTet.player.preview == MinoZ);

	mrsTet.state = TetrionReady;

	initialized = true;
	L.debug("Mrs initialized");
}

void mrsCleanup(void)
{
	if (!initialized) return;

	fieldDestroy(mrsTet.field);
	mrsTet.field = nullptr;

	initialized = false;
	L.debug("Mrs cleaned up");
}

/**
 * Populate and rotate the input arrays for press and hold detection.
 * @param inputs List of this frame's new inputs
 */
static void mrsUpdateInputs(span<Action> inputs)
{
	// Update raw inputs
	if (mrsTet.state != TetrionOutro) {
		for (size_t i = 0; i < inputs.size(); i += 1) {
			const Action& in = inputs[i];
			mrsTet.player.actionMapRaw[+in.type] = (in.state == Action::State::Pressed);
		}
	} else { // Force-release everything on gameover
		arrayClear(mrsTet.player.actionMapRaw);
	}

	// Rotate the input arrays
	arrayCopy(mrsTet.player.actionMapPrev, mrsTet.player.actionMap);
	arrayCopy(mrsTet.player.actionMap, mrsTet.player.actionMapRaw);

	// Filter conflicting inputs
	if (mrsTet.player.actionMap[+Action::Type::Lock] || mrsTet.player.actionMap[+Action::Type::Drop]) {
		mrsTet.player.actionMap[+Action::Type::Left] = false;
		mrsTet.player.actionMap[+Action::Type::Right] = false;
	}
	if (mrsTet.player.actionMap[+Action::Type::Left]
		&& mrsTet.player.actionMap[+Action::Type::Right]) {
		if (mrsTet.player.lastDirection == Action::Type::Left)
			mrsTet.player.actionMap[+Action::Type::Right] = false;
		if (mrsTet.player.lastDirection == Action::Type::Right)
			mrsTet.player.actionMap[+Action::Type::Left] = false;
	}

	// Update last direction
	if (inputHeld(Action::Type::Left))
		mrsTet.player.lastDirection = Action::Type::Left;
	else if (inputHeld(Action::Type::Right))
		mrsTet.player.lastDirection = Action::Type::Right;
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
	if (inputPressed(Action::Type::RotCW))
		rotate(1);
	if (inputPressed(Action::Type::RotCCW) || inputPressed(Action::Type::RotCCW2))
		rotate(-1);
}

/**
 * Shift the player piece, either through a direct press or autoshift.
 */
static void mrsUpdateShift(void)
{
	// Check requested movement direction
	int shiftDirection = 0;
	if (inputHeld(Action::Type::Left))
		shiftDirection = -1;
	else if (inputHeld(Action::Type::Right))
		shiftDirection = 1;

	// If not moving or moving in the opposite direction of ongoing DAS,
	// reset DAS and shift instantly
	if (!shiftDirection || shiftDirection != mrsTet.player.autoshiftDirection) {
		mrsTet.player.autoshiftDirection = shiftDirection;
		mrsTet.player.autoshiftCharge = 0;
		mrsTet.player.autoshiftDelay = MrsAutoshiftRepeat; // Starts out pre-charged
		if (shiftDirection && mrsTet.player.state == PlayerActive)
			shift(shiftDirection);
	}

	// If moving, advance and apply DAS
	if (!shiftDirection)
		return;
	if (mrsTet.player.autoshiftCharge < MrsAutoshiftCharge)
		mrsTet.player.autoshiftCharge += 1;
	if (mrsTet.player.autoshiftCharge == MrsAutoshiftCharge) {
		if (mrsTet.player.autoshiftDelay < MrsAutoshiftRepeat)
			mrsTet.player.autoshiftDelay += 1;

		// If during ARE, keep the DAS charged
		if (mrsTet.player.autoshiftDelay >= MrsAutoshiftRepeat
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
		mrsTet.player.spawnDelay + 1 == MrsClearOffset) {
		int clearedCount = checkClears();
		if (clearedCount) {
			mrsTet.player.state = PlayerClear;
			mrsTet.player.clearDelay = 0;
		}
	}

	// Advance counter, switch back to spawn delay if elapsed
	if (mrsTet.player.state == PlayerClear) {
		mrsTet.player.clearDelay += 1;
		if (mrsTet.player.clearDelay > MrsClearDelay) {
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
	if (mrsTet.state != TetrionPlaying || mrsDebugPauseSpawn)
		return; // Do not spawn during countdown or gameover
	if (mrsTet.player.state == PlayerSpawn
		|| mrsTet.player.state == PlayerNone) {
		mrsTet.player.spawnDelay += 1;
		if (mrsTet.player.spawnDelay >= MrsSpawnDelay)
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
	if (mrsTet.player.state != PlayerSpawned
		&& mrsTet.player.state != PlayerActive)
		return;

	int remainingGravity = mrsTet.player.gravity;
	if (mrsTet.player.state == PlayerActive) {
		if (inputHeld(Action::Type::Lock) || inputHeld(Action::Type::Drop))
			remainingGravity = FieldHeight * MrsSubGrid;
	}

	if (canDrop()) // Queue up the gravity drops
		mrsTet.player.ySub += remainingGravity;
	else
		mrsTet.player.ySub = 0;

	while (mrsTet.player.ySub >= MrsSubGrid) { // Drop until queue empty
		drop();
		mrsTet.player.ySub -= MrsSubGrid;
	}

	// Hard drop
	if (mrsTet.player.state == PlayerActive) {
		if (inputHeld(Action::Type::Lock))
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

	if (!mrsDebugInfLock)
		mrsTet.player.lockDelay += 1;
	// Two sources of locking: lock delay expired, manlock
	if (mrsTet.player.lockDelay > MrsLockDelay || inputHeld(Action::Type::Lock))
		lock();
}

/**
 * Win the game. Try to get this function called while playing.
 */
static void mrsUpdateWin(void)
{
	//TODO
}

void mrsAdvance(span<Action> inputs)
{
	ASSERT(initialized);

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
