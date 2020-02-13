/**
 * Implementation of pure.h
 * @file
 */

#include "pure.h"

#include <assert.h>
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

#define SoftDrop 256
#define DasCharge 16
#define DasDelay 1
#define LockDelay 30
#define ClearOffset 4
#define ClearDelay 41
#define SpawnDelay 30

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
	spin rotation;
	point2i pos;

	int dasDirection;
	int dasCharge;
	int dasDelay;
	int lockDelay;
	int clearDelay;
	int spawnDelay;
} Player;

/// A play's logical state
typedef struct Tetrion {
	Field* field;
	Player player;
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

void pureInit(void)
{
	if (initialized) return;

	// Logic init
	structClear(tet);
	tet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	tet.player.dasDelay = DasDelay; // Starts out pre-charged
	tet.player.spawnDelay = SpawnDelay; // Start instantly

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
 * @param direction 1 for clockwise, -1 for counter-clockwise
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

static void shift(int direction)
{
	tet.player.pos.x += direction;
	if (pieceOverlapsField(getPiece(tet.player.type, tet.player.rotation),
		tet.player.pos, tet.field)) {
		tet.player.pos.x -= direction;
	}
}

static void spawnPiece(void)
{
	tet.player.state = PlayerSpawned; // Some moves restricted on first frame
	tet.player.pos.x = SpawnX;
	tet.player.pos.y = SpawnY;

	// Picking the next piece
//	tet.player.type = tet.player.preview;
//	tet.player.preview = randomPiece();
	tet.player.type = MinoT;

	if (tet.player.type == MinoI)
		tet.player.pos.y -= 1; // I starts higher than other pieces
//	tet.player.ySub = 0;
	tet.player.lockDelay = 0;
	tet.player.spawnDelay = 0;
	tet.player.clearDelay = 0;
	tet.player.rotation = SpinNone;
//	tet.player.dropBonus = 0;

	// IRS
	if (inputHeld(InputButton2)) {
		rotate(-1);
	} else {
		if (inputHeld(InputButton1) || inputHeld(InputButton3))
			rotate(1);
	}

//	addLevels(1, false);

//	if (!checkPosition())
//		gameOver();
}

/**
 * Populate and rotate the input arrays for press and hold detection.
 * @param inputs List of this frame's new inputs
 */
static void pureUpdateInputs(darray* inputs)
{
	assert(inputs);

	// Update raw inputs
	for (size_t i = 0; i < darraySize(inputs); i += 1) {
		Input* in = darrayGet(inputs, i);
		assert(in->type < InputSize);
		tet.player.inputMapRaw[in->type] = in->state;
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

/**
 * Spawn a new piece if needed.
 */
static void pureUpdateSpawn(void)
{
//	if (game->state != GameplayPlaying)
//		return; // Do not spawn during countdown or gameover
	if (tet.player.state == PlayerSpawn || tet.player.state == PlayerNone) {
		tet.player.spawnDelay += 1;
		if (tet.player.spawnDelay >= SpawnDelay)
			spawnPiece();
	}
}

void pureAdvance(darray* inputs)
{
	assert(inputs);
	assert(initialized);

	pureUpdateInputs(inputs);
	pureUpdateState();
	pureUpdateRotation();
	pureUpdateShift();
//	pureUpdateClear();
	pureUpdateSpawn();
//	pureUpdateGhost();
//	pureUpdateGravity();
//	pureUpdateLocking();
//	pureUpdateWin();
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
//	pureDrawBorder();
//  pureDrawStats();
}
