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

// Frames until the first autoshift
// 2 is right after the normal shift
#define DAS_CHARGE 16

// Number of frames between autoshifts
// 1 is every frame
// 0 is instant (not supported yet)
#define DAS_DELAY 1

// Subgrid value at which the piece is dropped
#define SUBGRID 256

// Natural piece falling speed in subgrids
int GRAVITY = 0;

// Piece falling speed if soft drop is held
#define SOFT_DROP 256

// Piece falling speed if sonic drop is held
#define SONIC_DROP 5120

// How many frames a piece takes to lock if it can't drop
#define LOCK_DELAY 30

// How many frames it takes for full lines to clear
#define CLEAR_DELAY 41

// How many frames it takes for the next piece to spawn
// after the previous one is locked
#define SPAWN_DELAY 30

// The number of times the randomizes attempts to pick a piece not in history
#define MAX_REROLLS 4

// Convenience pointer for storing app->game
static struct game *game = NULL;

// Convenience pointer for scoring app->game.player
static struct player *player = NULL;

// RNG specifically for next piece selection
static rng randomizer = {};

// Accepts inputs outside of bounds
enum mino getGrid(int x, int y)
{
	if (x < 0 || x >= PLAYFIELD_W || y >= PLAYFIELD_H)
		return MinoGarbage;
	if (y < 0)
		return MinoNone;
	return game->playfield[y][x];
}

static void setGrid(int x, int y, enum mino val)
{
	if (x < 0 || x >= PLAYFIELD_W ||
	    y < 0 || y >= PLAYFIELD_H)
		return;
	game->playfield[y][x] = val;
}

// Check that player's position doesn't overlap the playfield
static bool checkPosition(void)
{
	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		int x = player->x + rs[player->type][player->rotation][i].x;
		int y = player->y + rs[player->type][player->rotation][i].y;
		if (getGrid(x, y))
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

	if (player->state == PlayerSpawned)
		return false; // If this is IRS, don't attempt kicks
	if (player->type == PieceI)
		return false; // I doesn't kick

	// The annoying special treatment of LTJ middle column
	if (player->rotation % 2 == 1 && // Vertical orientation
	    (player->type == PieceL ||
	     player->type == PieceT ||
	     player->type == PieceJ)) {
		bool success = true;
		for (int i = 0; i < MINOS_PER_PIECE; i++) {
			int xLocal = rs[player->type][player->rotation][i].x;
			int x = player->x
			        + rs[player->type][player->rotation][i].x;
			int y = player->y
			        + rs[player->type][player->rotation][i].y;
			if (xLocal != CENTER_COLUMN && getGrid(x, y)) {
				success = true;
				break;
			}
			if (getGrid(x, y))
				success = false;
		}
		if (!success)
			return false;
	}

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
		if (game->cmdHeld[CmdSoft])
			player->dropBonus += 1;
	}
}

// Stamp player piece into the playfield
// Does not do collision checking, so can overwrite filled grids
static void lock(void)
{
	if (game->cmdHeld[CmdSoft])
		player->dropBonus += 1;
	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		int x = player->x + rs[player->type][player->rotation][i].x;
		int y = player->y + rs[player->type][player->rotation][i].y;
		if (player->y + rs[player->type][player->rotation][i].y < 0)
			continue;
		setGrid(x, y, (enum mino)player->type);
	}
	player->state = PlayerSpawn;
}

static enum pieceType randomPiece(void)
{
	bool first = false;
	if (player->history[0] == PieceNone) { // History empty, initialize
		first = true;
		for (int i = 0; i < HISTORY_SIZE; i++)
			player->history[i] = PieceZ;
	}

	enum pieceType result = PieceNone;
	for (int i = 0; i < MAX_REROLLS; i++) {
		result = random(&randomizer, PieceSize - 1) + 1;
		while (first && // Unfair first piece prevention
		       (result == PieceS ||
		        result == PieceZ ||
		        result == PieceO))
			result = random(&randomizer, PieceSize - 1) + 1;

		bool valid = true;
		for (int j = 0; j < HISTORY_SIZE; j++) {
			if (result == player->history[j])
				valid = false;
		}
		if (valid)
			break;
	}

	for (int i = HISTORY_SIZE - 2; i >= 0; i--) {
		player->history[i + 1] = player->history[i];
	}
	player->history[0] = result;
	return result;
}

static void adjustGravity(void)
{
	if (game->level < 30)
		GRAVITY = 4;
	else if (game->level < 35)
		GRAVITY = 6;
	else if (game->level < 40)
		GRAVITY = 8;
	else if (game->level < 50)
		GRAVITY = 10;
	else if (game->level < 60)
		GRAVITY = 12;
	else if (game->level < 70)
		GRAVITY = 16;
	else if (game->level < 80)
		GRAVITY = 32;
	else if (game->level < 90)
		GRAVITY = 48;
	else if (game->level < 100)
		GRAVITY = 64;
	else if (game->level < 120)
		GRAVITY = 80;
	else if (game->level < 140)
		GRAVITY = 96;
	else if (game->level < 160)
		GRAVITY = 112;
	else if (game->level < 170)
		GRAVITY = 128;
	else if (game->level < 200)
		GRAVITY = 144;
	else if (game->level < 220)
		GRAVITY = 4;
	else if (game->level < 230)
		GRAVITY = 32;
	else if (game->level < 233)
		GRAVITY = 64;
	else if (game->level < 236)
		GRAVITY = 96;
	else if (game->level < 239)
		GRAVITY = 128;
	else if (game->level < 243)
		GRAVITY = 160;
	else if (game->level < 247)
		GRAVITY = 192;
	else if (game->level < 251)
		GRAVITY = 224;
	else if (game->level < 300)
		GRAVITY = 256;
	else if (game->level < 330)
		GRAVITY = 512;
	else if (game->level < 360)
		GRAVITY = 768;
	else if (game->level < 400)
		GRAVITY = 1024;
	else if (game->level < 420)
		GRAVITY = 1280;
	else if (game->level < 450)
		GRAVITY = 1024;
	else if (game->level < 500)
		GRAVITY = 768;
	else
		GRAVITY = 5120;
}

static void addLevels(int count, bool strong)
{
	game->level += count;
	if (!strong && game->level >= game->nextLevelstop)
		game->level = game->nextLevelstop - 1;
	else if (game->level >= game->nextLevelstop)
		game->nextLevelstop += 100;
	if (game->nextLevelstop > 900)
		game->nextLevelstop = 999;

	adjustGravity();
}

// Generate a new random piece for the player to control
static void newPiece(void)
{
	player->state = PlayerSpawned;
	player->x = PLAYFIELD_W / 2 - PIECE_BOX / 2; // Centered
	player->y = -2 + PLAYFIELD_H_HIDDEN;

	// Picking the next piece
	bool first = false;
	if (player->preview == PieceNone) {
		player->preview = randomPiece();
		first = true;
	}
	player->type = player->preview;
	player->preview = randomPiece();

	if (player->type == PieceI)
		player->y += 1;
	player->ySub = 0;
	player->lockDelay = 0;
	player->spawnDelay = 0;
	player->rotation = 0;
	player->dropBonus = 0;

	// IRS
	if (game->cmdHeld[CmdCW]) {
		rotate(1);
	} else {
		if (game->cmdHeld[CmdCCW] || game->cmdHeld[CmdCCW2])
			rotate(-1);
	}

	if (!first)
		addLevels(1, false);
}

// Maps generic inputs to gameplay commands
static enum cmdType inputToCmd(enum inputType i)
{
	switch (i) {
	case InputLeft:
		return CmdLeft;
	case InputRight:
		return CmdRight;
//	case InputUp:
//		return CmdSonic;
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

// Receive unfiltered inputs
static void processInput(struct input *i)
{
	switch (i->action) {
	case ActionPressed:
		// Quitting is handled outside of gameplay logic
		if (i->type == InputQuit) {
			logInfo("User exited");
			setRunning(false);
			break;
		}

		if (inputToCmd(i->type) != CmdNone)
			game->cmdUnfiltered[inputToCmd(i->type)] = true;
		break;

	case ActionReleased:
		if (inputToCmd(i->type) != CmdNone)
			game->cmdUnfiltered[inputToCmd(i->type)] = false;
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

	// Receive all pending inputs
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processInput(in);
		free(in);
	}

	// Passthrough the inputs
	for (int i = CmdNone + 1; i < CmdSize; i++) {
		if (game->cmdUnfiltered[i] && !game->cmdHeld[i]) {
			game->cmdPressed[i] = true;
			game->cmdHeld[i] = true;
		} else if (!game->cmdUnfiltered[i] && game->cmdHeld[i]) {
			game->cmdHeld[i] = false;
		}
	}

	// Filter the conflicting ones
	if ((game->cmdHeld[CmdLeft] && game->cmdHeld[CmdRight]) ||
	    game->cmdHeld[CmdSoft] || game->cmdHeld[CmdSonic]) {
		game->cmdPressed[CmdLeft] = false;
		game->cmdHeld[CmdLeft] = false;
		game->cmdPressed[CmdRight] = false;
		game->cmdHeld[CmdRight] = false;
	}
}

void initGameplay(void)
{
	srandom(&randomizer, (uint64_t)time(NULL));

	game = allocate(sizeof(*game));
	app->game = game;
	for (int y = 0; y < PLAYFIELD_H; y++) {
		for (int x = 0; x < PLAYFIELD_W; x++)
			setGrid(x, y, MinoNone);
		game->clearedLines[y] = false;
	}
	for (int i = 0; i < CmdSize; i++) {
		game->cmdUnfiltered[i] = false;
		game->cmdPressed[i] = false;
		game->cmdHeld[i] = false;
	}
	game->level = 0;
	game->nextLevelstop = 100;
	game->score = 0;
	player = &game->player;
	player->state = PlayerNone;
	player->x = 0;
	player->y = 0;
	player->ySub = 0;
	player->type = PieceNone;
	player->preview = PieceNone;
	for (int i = 0; i < HISTORY_SIZE; i++)
		player->history[i] = PieceNone;
	player->rotation = 0;
	player->dasDirection = 0;
	player->dasCharge = 0;
	player->dasDelay = DAS_DELAY; // Starts out pre-charged
	player->lockDelay = 0;
	player->clearDelay = 0;
	player->spawnDelay = SPAWN_DELAY; // Start instantly
	player->dropBonus = 0;
	adjustGravity();
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
	if (player->state != PlayerActive)
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
	if (!shiftDirection || shiftDirection != player->dasDirection) {
		player->dasDirection = shiftDirection;
		player->dasCharge = 0;
		player->dasDelay = DAS_DELAY; // Starts out pre-charged
		if (shiftDirection && player->state == PlayerActive)
			shift(shiftDirection);
	}

	// If moving, advance and apply DAS
	if (!shiftDirection)
		return;
	if (player->dasCharge < DAS_CHARGE)
		player->dasCharge += 1;
	if (player->dasCharge == DAS_CHARGE) {
		if (player->dasDelay < DAS_DELAY)
			player->dasDelay += 1;

		// If during ARE, keep the DAS charged
		if (player->dasDelay >= DAS_DELAY
		    && player->state == PlayerActive) {
			player->dasDelay = 0;
			shift(player->dasDirection);
		}
	}
}

static int checkClears(void)
{
	int count = 0;
	for (int y = 0; y < PLAYFIELD_H; y++) {
		bool isCleared = true;
		for (int x = 0; x < PLAYFIELD_W; x++) {
			if (!getGrid(x, y)) {
				isCleared = false;
				break;
			}
		}
		if (!isCleared)
			continue;
		count += 1;
		game->clearedLines[y] = true;
		for (int x = 0; x < PLAYFIELD_W; x++)
			setGrid(x, y, MinoNone);
	}
	return count;
}

static void thump(void)
{
	for (int y = 0; y < PLAYFIELD_H; y++) {
		if (!game->clearedLines[y])
			continue;
		for (int yy = y; yy >= 0; yy--) {
			for (int xx = 0; xx < PLAYFIELD_W; xx++)
				setGrid(xx, yy, getGrid(xx, yy - 1));
		}
		game->clearedLines[y] = false;
	}
}

static void addScore(int lines)
{
	int score;
	score = game->level + lines;
	int remainder = score % 4;
	score /= 4;
	if (remainder) score += 1;
	score += player->dropBonus;
	score *= lines;
	//score *= game->combo;
	int bravo = 4;
	for (int y = 0; y < PLAYFIELD_H; y++) {
		for (int x = 0; x < PLAYFIELD_W; x++) {
			if (game->playfield[y][x] != MinoNone) {
				bravo = 1;
				goto bravoOut;
			}
		}
	}
bravoOut:
	score *= bravo;

	game->score += score;
}

static void updateClear(void)
{
	if (player->state == PlayerSpawn && player->spawnDelay == 0) {
		int clearedCount = checkClears();
		if (clearedCount) {
			player->state = PlayerClear;
			player->clearDelay = 0;
			addScore(clearedCount);
			addLevels(clearedCount, true);
		}
	}

	if (player->state == PlayerClear) {
		player->clearDelay += 1;
		if (player->clearDelay >= CLEAR_DELAY) {
			thump();
			player->state = PlayerSpawn;
		}
	}
}

static void updateSpawn(void)
{
	if (player->state == PlayerSpawn || player->state == PlayerNone) {
		player->spawnDelay += 1;
		if (player->spawnDelay >= SPAWN_DELAY)
			newPiece();
	}
}

static void updateGravity()
{
	if (player->state != PlayerSpawned && player->state != PlayerActive)
		return;
	int gravity = GRAVITY;
	if (player->state == PlayerActive) {
		if (game->cmdHeld[CmdSoft] && gravity < SOFT_DROP)
			gravity = SOFT_DROP;
		if (game->cmdHeld[CmdSonic])
			gravity = SONIC_DROP;
	}
	player->ySub += gravity;
	while (player->ySub >= SUBGRID) {
		drop();
		player->ySub -= SUBGRID;
	}
}

void updateLocking(void)
{
	if (player->state != PlayerActive || canDrop())
		return;
	player->lockDelay += 1;
	// Two sources of locking: lock delay expired, and manlock
	if (player->lockDelay >= LOCK_DELAY || game->cmdHeld[CmdSoft])
		lock();
}

void updateGameplay(void)
{
	processInputs();

	updateRotations();
	updateShifts();
	updateClear();
	updateSpawn();
	updateGravity();
	updateLocking();

	if (player->state == PlayerSpawned)
		player->state = PlayerActive;
}
