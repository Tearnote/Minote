/**
 * Implementation of pure.h
 * @file
 */

#include "pure.h"

#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "renderer.h"
#include "mapper.h"
#include "mino.h"
#include "util.h"
#include "log.h"

#define FieldWidth 10u ///< Width of #field
#define FieldHeight 22u ///< Height of #field
#define FieldHeightVisible 20u ///< Number of bottom rows the player can see

#define SpawnX 3 ///< X position of player piece spawn
#define SpawnY 18 ///< Y position of player piece spawn
#define SubGrid 256

#define HistorySize 4
#define MaxRerolls 4

#define ClockFrequency 60.0
#define ClockTick (secToNsec(1) / ClockFrequency)

#define SoftDrop 256
#define DasCharge 16
#define DasDelay 1
#define LockDelay 30
#define ClearOffset 4
#define ClearDelay 41
#define SpawnDelay 30

typedef struct Threshold {
	int level;
	int gravity;
} Threshold;

#define Thresholds ((Threshold[]){     \
    { .level = 0, .gravity = 4 },      \
    { .level = 30, .gravity = 6 },     \
    { .level = 35, .gravity = 8 },     \
    { .level = 40, .gravity = 10 },    \
    { .level = 50, .gravity = 12 },    \
    { .level = 60, .gravity = 16 },    \
    { .level = 70, .gravity = 32 },    \
    { .level = 80, .gravity = 48 },    \
    { .level = 90, .gravity = 64 },    \
    { .level = 100, .gravity = 80 },   \
    { .level = 120, .gravity = 96 },   \
    { .level = 140, .gravity = 112 },  \
    { .level = 160, .gravity = 128 },  \
    { .level = 170, .gravity = 144 },  \
    { .level = 200, .gravity = 4 },    \
    { .level = 220, .gravity = 32 },   \
    { .level = 230, .gravity = 64 },   \
    { .level = 233, .gravity = 96 },   \
    { .level = 236, .gravity = 128 },  \
    { .level = 239, .gravity = 160 },  \
    { .level = 243, .gravity = 192 },  \
    { .level = 247, .gravity = 224 },  \
    { .level = 251, .gravity = 256 },  \
    { .level = 300, .gravity = 512 },  \
    { .level = 330, .gravity = 768 },  \
    { .level = 360, .gravity = 1024 }, \
    { .level = 400, .gravity = 1280 }, \
    { .level = 420, .gravity = 1024 }, \
    { .level = 450, .gravity = 768 },  \
    { .level = 500, .gravity = 5120 }  \
})

typedef struct Requirement {
	int level;
	int score;
	nsec time;
} Requirement;

#define Requirements ((Requirement[]){                                \
    { .level = 300, .score = 12000,  .time = secToNsec(4 * 60 + 15)}, \
    { .level = 500, .score = 40000,  .time = secToNsec(7 * 60)},      \
    { .level = 999, .score = 126000, .time = secToNsec(13 * 60 + 30)} \
})

#define Grades ((int[]){ \
    0,                   \
    400,                 \
    800,                 \
    1400,                \
    2000,                \
    3500,                \
    5500,                \
    8000,                \
    12000,               \
    16000,               \
    22000,               \
    30000,               \
    40000,               \
    52000,               \
    66000,               \
    82000,               \
    100000,              \
    120000,              \
    126000               \
})

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
	mino type;
	mino preview;
	mino history[HistorySize];
	spin rotation;
	point2i pos;
	int ySub;

	int dasDirection;
	int dasCharge;
	int dasDelay;
	int lockDelay;
	int clearDelay;
	int spawnDelay;

	int level;
	int dropBonus;
} Player;

typedef enum TetrionState {
	TetrionNone,
	TetrionReady,
	TetrionPlaying,
	TetrionOutro,
	TetrionSize
} TetrionState;

typedef enum ReqStatus {
	ReqNone,
	ReqPassed,
	ReqFailed,
	ReqSize
} ReqStatus;

/// A play's logical state
typedef struct Tetrion {
	TetrionState state;
	int ready;
	int frame;

	Field* field;
	bool linesCleared[FieldHeight];
	Player player;
	rng rngState;

	int score;
	int combo;
	int grade;
	ReqStatus reqs[countof(Requirements)];
} Tetrion;

/// Full state of the mode
static Tetrion tet = {0};

static Model* scene = null;

static Model* block = null;
static darray* tints = null; ///< of #block
static darray* transforms = null; ///< of #block

static bool initialized = false;

#define inputPressed(type) \
    (tet.player.inputMap[(type)] && !tet.player.inputMapPrev[(type)])

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
		result = random(&tet.rngState, MinoGarbage - 1) + 1;
		while (first && // Unfair first piece prevention
			(result == MinoS ||
				result == MinoZ ||
				result == MinoO))
			result = random(&tet.rngState, MinoGarbage - 1) + 1;

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

static int getLevelstop(int level)
{
	int result = (level / 100 + 1) * 100;
	if (result >= 1000)
		result = 999;
	return result;
}

static void addLevels(int count, bool strong)
{
	int levelstop = getLevelstop(count);
	tet.player.level += count;
	if (!strong && tet.player.level >= levelstop)
		tet.player.level = levelstop - 1;
}

static void gameOver(void)
{
	tet.state = TetrionOutro;
}

static void spawnPiece(void)
{
	tet.player.state = PlayerSpawned; // Some moves restricted on first frame
	tet.player.pos.x = SpawnX;
	tet.player.pos.y = SpawnY;

	// Picking the next piece
	tet.player.type = tet.player.preview;
	tet.player.preview = randomPiece();

	if (tet.player.type == MinoI)
		tet.player.pos.y -= 1; // I starts higher than other pieces
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
 * Check for triggers and progress through phases.
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
	if (!shiftDirection || shiftDirection != tet.player.dasDirection) {
		tet.player.dasDirection = shiftDirection;
		tet.player.dasCharge = 0;
		tet.player.dasDelay = DasDelay; // Starts out pre-charged
		if (shiftDirection && tet.player.state == PlayerActive)
			shift(shiftDirection);
	}

	// If moving, advance and apply DAS
	if (!shiftDirection)
		return;
	if (tet.player.dasCharge < DasCharge)
		tet.player.dasCharge += 1;
	if (tet.player.dasCharge == DasCharge) {
		if (tet.player.dasDelay < DasDelay)
			tet.player.dasDelay += 1;

		// If during ARE, keep the DAS charged
		if (tet.player.dasDelay >= DasDelay
			&& tet.player.state == PlayerActive) {
			tet.player.dasDelay = 0;
			shift(tet.player.dasDirection);
		}
	}
}

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

static nsec getClock(int frame)
{
	return frame * ClockTick;
}

static void updateRequirements(void)
{
	assert(countof(tet.reqs) == countof(Requirements));
	for (size_t i = 0; i < countof(tet.reqs); i += 1) {
		if (tet.reqs[i])
			continue; // Only check each treshold once, when reached
		if (tet.player.level < Requirements[i].level)
			return; // Threshold not reached yet
		if (tet.score >= Requirements[i].score &&
			getClock(tet.frame) <= Requirements[i].time)
			tet.reqs[i] = ReqPassed;
		else
			tet.reqs[i] = ReqFailed;
	}
}

static bool requirementsMet(void)
{
	for (size_t i = 0; i < countof(tet.reqs); i += 1) {
		if (tet.reqs[i] != ReqPassed)
			return false;
	}
	return true;
}

static void updateGrade(void)
{
	for (size_t i = 0; i < countof(Grades); i += 1) {
		if (tet.score < Grades[i])
			return;
		if (i == countof(Grades) - 1 &&
			(!requirementsMet() || tet.player.level < 999))
			return; // Final grade, requirements not met
		tet.grade = i;
	}
}

static void thump(void)
{
	for (int y = FieldHeight - 1; y >= 0; y -= 1) {
		if (!tet.linesCleared[y])
			continue; // Drop only above cleared lines
		fieldDropRow(tet.field, y);
		tet.linesCleared[y] = false;
	}
}

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

static int getGravity(int level)
{
	int result = 0;
	for (int i = 0; i < countof(Thresholds); i += 1) {
		if (level < Thresholds[i].level)
			break;
		result = Thresholds[i].gravity;
	}
	return result;
}

static bool canDrop(void)
{
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	return !pieceOverlapsField(playerPiece, (point2i){
		.x = tet.player.pos.x,
		.y = tet.player.pos.y - 1
	}, tet.field);
}

static void drop(void)
{
	if (!canDrop())
		return;

	tet.player.lockDelay = 0;
	tet.player.pos.y -= 1;
	if (inputHeld(InputDown))
		tet.player.dropBonus += 1;
}

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

static void lock(void)
{
	if (inputHeld(InputDown))
		tet.player.dropBonus += 1; // Lock frame can also increase this
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	fieldStampPiece(tet.field, playerPiece, tet.player.pos, tet.player.type);
	tet.player.state = PlayerSpawn;
}

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

static void pureUpdateWin(void)
{
	if (tet.player.level >= 999)
		gameOver();
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
	srandom(&tet.rngState, (uint64_t)time(NULL));
	tet.player.level = -1;
	tet.player.dasDelay = DasDelay; // Starts out pre-charged
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
	tints = darrayCreate(sizeof(color4));
	transforms = darrayCreate(sizeof(mat4x4));

	initialized = true;
	logDebug(applog, u8"Pure sublayer initialized");
}

void pureCleanup(void)
{
	if (!initialized) return;
	if (transforms) {
		darrayDestroy(transforms);
		transforms = null;
	}
	if (tints) {
		darrayDestroy(tints);
		tints = null;
	}
	if (block) {
		modelDestroy(block);
		block = null;
	}
	if (scene) {
		modelDestroy(scene);
		scene = null;
	}
	if (tet.field) {
		fieldDestroy(tet.field);
		tet.field = null;
	}
	initialized = false;
	logDebug(applog, u8"Pure sublayer cleaned up");
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
 * Draw the contents of the tetrion field.
 */
static void pureDrawField(void)
{
	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;
		// Flip the order of processing the left half of the playfield
		// to fix alpha sorting issues
		if (x < FieldWidth / 2)
			x = FieldWidth / 2 - x - 1;
		mino type = fieldGet(tet.field, (point2i){x, y});
		if (type == MinoNone) continue;

		color4* tint = darrayProduce(tints);
		mat4x4* transform = darrayProduce(transforms);
		color4Copy(*tint, minoColor(type));
		if (y >= FieldHeightVisible)
			tint->a /= 4.0f;
		mat4x4_identity(*transform);
		mat4x4_translate_in_place(*transform, x - (signed)(FieldWidth / 2), y,
			0.0f);
	}
	modelDraw(block, darraySize(transforms), darrayData(tints),
		darrayData(transforms));
	darrayClear(tints);
	darrayClear(transforms);
}

/**
 * Draw the player piece on top of the field.
 */
static void pureDrawPlayer(void)
{
	if (tet.player.state != PlayerActive &&
		tet.player.state != PlayerSpawned)
		return;
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = (*playerPiece)[i].x + tet.player.pos.x;
		int y = (*playerPiece)[i].y + tet.player.pos.y;

		color4* tint = darrayProduce(tints);
		mat4x4* transform = darrayProduce(transforms);
		color4Copy(*tint, minoColor(tet.player.type));
		mat4x4_identity(*transform);
		mat4x4_translate_in_place(*transform, x - (signed)(FieldWidth / 2), y,
			0.0f);
	}
	modelDraw(block, darraySize(transforms), darrayData(tints),
		darrayData(transforms));
	darrayClear(tints);
	darrayClear(transforms);
}

void pureDraw(void)
{
	assert(initialized);

	rendererClear((color3){0.010f, 0.276f, 0.685f}); //TODO make into layer
	pureDrawScene();
	pureDrawField();
	pureDrawPlayer();
//	pureDrawGhost();
//  pureDrawPreview();
//	pureDrawBorder();
//  pureDrawStats();
}
