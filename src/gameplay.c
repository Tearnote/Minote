// Minote - gameplay.c

#include "gameplay.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>

#include "state.h"
#include "input.h"
#include "mino.h"
#include "log.h"
#include "util.h"

// Convenience pointer for storing app->game
static struct gameState *game = NULL;

// Convenience pointer for scoring app->game.player
static struct playerState *player = NULL;

// RNG specifically for next piece selection
static rng randomizer = {};

// Check a single playfield cell
// Accepts inputs outside of bounds
static bool isFree(int x, int y)
{
	if (x < 0 || x >= PLAYFIELD_W || y >= PLAYFIELD_H)
		return false;
	if (y < 0)
		return true;
	return (bool)(game->playfield[y][x] == MinoNone);
}

// Check that player's position doesn't overlap the playfield
static bool checkPosition(void)
{
	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		int x = player->x + rs[player->type][player->rotation][i].x;
		int y = player->y + rs[player->type][player->rotation][i].y;
		if (!isFree(x, y))
			return false;
	}
	return true;
}

// Verify that player's position is legal, attempt kicks otherwise
// Returns true if successful, false if last move needs to be reverted
static bool tryKicks(void)
{
	static int preference = 1;
	if (checkPosition())
		return true; // Original position
	player->x += preference;
	if (checkPosition())
		return true; // 1 to the right
	player->x -= preference * 2;
	if (checkPosition())
		return true; // 1 to the left
	player->x += preference;
	return false; // Failure, returned to original position
}

// Attempt to move player piece sideways
// -1 is left
// 1 is right
static void shift(int direction)
{
	player->x += direction;
	if (!checkPosition())
		player->x -= direction;
}

// Attempt to rotate player piece
// 1 is CW
// -1 is CCW
static void rotate(int direction)
{
	int prevRotation = player->rotation;
	player->rotation += direction;
	if (player->rotation >= 4)
		player->rotation -= 4;
	if (player->rotation < 0)
		player->rotation += 4;
	if (!tryKicks())
		player->rotation = prevRotation;
}

// Check whether player piece can drop one grid
static bool canDrop(void)
{
	player->y += 1;
	bool result = checkPosition();
	player->y -= 1;
	return result;
}

// Move one grid downwards, if possible
static void drop(void)
{
	if (canDrop()) {
		player->y += 1;
		player->lockDelay = 0;
	}
}

// Stamp player piece into the playfield
// Does not do collision checking, so can overwrite filled grids
static void lock(void)
{
	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		int x = player->x + rs[player->type][player->rotation][i].x;
		int y = player->y + rs[player->type][player->rotation][i].y;
		if (player->y + rs[player->type][player->rotation][i].y < 0)
			continue;
		game->playfield[y][x] = (enum mino)player->type;
	}
	player->exists = false;
}

// Generate a new random piece for the player to control
static void newPiece(void)
{
	player->exists = true;
	player->x = PLAYFIELD_W / 2 - PIECE_BOX / 2; // Centered
	player->y = -2;

	// Picking the next piece
	if (player->preview == PieceNone)
		player->preview = random(&randomizer, PieceSize - 1) + 1;
	player->type = player->preview;
	player->preview = random(&randomizer, PieceSize - 1) + 1;

	if (player->type == PieceI)
		player->y += 1;
	player->ySub = 0;
	player->lockDelay = 0;
	player->spawnDelay = 0;
	player->rotation = 0;

	// IRS
	if (game->cmdHeld[CmdCW]) {
		rotate(1);
	} else {
		if (game->cmdHeld[CmdCCW] || game->cmdHeld[CmdCCW2])
			rotate(-1);
	}
}

// Maps generic inputs to gameplay commands
static enum cmdType inputToCmd(enum inputType i)
{
	switch (i) {
	case InputLeft:
		return CmdLeft;
	case InputRight:
		return CmdRight;
	case InputUp:
		return CmdSonic;
	case InputDown:
		return CmdSoft;
	case InputButton1:
		return CmdCCW;
	case InputButton2:
		return CmdCW;
	case InputButton3:
		return CmdCCW2;
	default:
		return CmdNone;
	}
}

void processInput(struct input *i)
{
	switch (i->action) {
	case ActionPressed:
		// Quitting is handled outside of gameplay logic
		if (i->type == InputQuit) {
			logInfo("User exited");
			setRunning(false);
			break;
		}
		// 4-way emulation
		if (i->type == InputUp || i->type == InputDown ||
		    i->type == InputLeft || i->type == InputRight) {
			game->cmdPressed[inputToCmd(InputUp)] = false;
			game->cmdPressed[inputToCmd(InputDown)] = false;
			game->cmdPressed[inputToCmd(InputLeft)] = false;
			game->cmdPressed[inputToCmd(InputRight)] = false;
			game->cmdHeld[inputToCmd(InputUp)] = false;
			game->cmdHeld[inputToCmd(InputDown)] = false;
			game->cmdHeld[inputToCmd(InputLeft)] = false;
			game->cmdHeld[inputToCmd(InputRight)] = false;
		}

		if (inputToCmd(i->type) != CmdNone) {
			game->cmdPressed[inputToCmd(i->type)] = true;
			game->cmdHeld[inputToCmd(i->type)] = true;
		}
		break;
	case ActionReleased:
		if (inputToCmd(i->type) != CmdNone) {
			game->cmdPressed[inputToCmd(i->type)] = false;
			game->cmdHeld[inputToCmd(i->type)] = false;
		}
		break;
	default:
		break;
	}
}

// Polls for inputs and transforms them into gameplay commands
static void processInputs(void)
{
	// Clear previous frame's presses
	for (int i = 0; i < CmdSize; i++)
		game->cmdPressed[i] = false;

	struct input *i = NULL;
	while ((i = dequeueInput())) {
		processInput(i);
		free(i);
	}
}

void initGameplay(void)
{
	srandom(&randomizer, (uint64_t)time(NULL));

	game = allocate(sizeof(*game));
	app->game = game;
	for (int y = 0; y < PLAYFIELD_H; y++)
		for (int x = 0; x < PLAYFIELD_W; x++)
			game->playfield[y][x] = MinoNone;
	for (int i = 0; i < CmdSize; i++) {
		game->cmdPressed[i] = false;
		game->cmdHeld[i] = false;
	}
	player = &game->player;
	player->exists = false;
	player->x = 0;
	player->y = 0;
	player->ySub = 0;
	player->type = PieceNone;
	player->preview = PieceNone;
	player->rotation = 0;
	player->dasDirection = 0;
	player->dasCharge = 0;
	player->dasDelay = DAS_DELAY; // Starts out pre-charged
	player->lockDelay = 0;
	player->spawnDelay = SPAWN_DELAY - 1; // Start instantly
}

void cleanupGameplay(void)
{
	player = NULL;
	free(game);
	game = NULL;
	app->game = NULL;
}

static void updateRotations(void)
{
	if (!player->exists)
		return;
	if (game->cmdPressed[CmdCW])
		rotate(1);
	if (game->cmdPressed[CmdCCW] || game->cmdPressed[CmdCCW2])
		rotate(-1);
}

static void updateShifts(void)
{
	// Check requested movement direction
	int shiftDirection = 0;
	if (game->cmdHeld[CmdLeft])
		shiftDirection = -1;
	else if (game->cmdHeld[CmdRight])
		shiftDirection = 1;

	// If not moving or moving in the opposite direction of ongoing DAS,
	// reset DAS and shift instantly
	if (shiftDirection == 0 || shiftDirection != player->dasDirection) {
		player->dasDirection = shiftDirection;
		player->dasCharge = 0;
		player->dasDelay = DAS_DELAY; // Starts out pre-charged
		if (shiftDirection && player->exists)
			shift(shiftDirection);
	}

	// If moving, advance and apply DAS
	if (!shiftDirection)
		return;
	if (player->dasCharge < DAS_CHARGE)
		player->dasCharge += 1;
	if (player->dasCharge == DAS_CHARGE) {
		if (player->dasDelay < DAS_DELAY && player->exists)
			player->dasDelay += 1;

		// If during ARE, keep the DAS charged
		if (player->dasDelay == DAS_DELAY && player->exists) {
			player->dasDelay = 0;
			shift(player->dasDirection);
		}
	}
}

static int calculateGravity(void)
{
	int gravity = GRAVITY;
	if (player->exists) {
		if (game->cmdHeld[CmdSoft] && gravity < SOFT_DROP)
			gravity = SOFT_DROP;
		if (game->cmdPressed[CmdSonic])
			gravity = SONIC_DROP;
	}
	return gravity;
}

static void updateSpawn(void)
{
	if (player->exists)
		return;
	player->spawnDelay += 1;
	if (player->spawnDelay == SPAWN_DELAY)
		newPiece();
}

static void updateGravity(int gravity)
{
	if (!player->exists)
		return;
	player->ySub += gravity;
	while (player->ySub >= SUBGRID) {
		drop();
		player->ySub -= SUBGRID;
	}
}

void updateLocking(void)
{
	if (!player->exists || canDrop())
		return;
	player->lockDelay += 1;
	// Two sources of locking: lock delay expired, and manlock
	if (player->lockDelay == LOCK_DELAY || game->cmdHeld[CmdSoft])
		lock();
}

void updateGameplay(void)
{
	processInputs();

	updateRotations();
	updateShifts();
	// We need to calculate gravity before spawn,
	// so that we can't soft/sonic drop on the first frame of a new piece
	int gravity = calculateGravity();
	updateSpawn();
	updateGravity(gravity);
	updateLocking();
}
