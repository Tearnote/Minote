// Minote - gameplay.c

#include "gameplay.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>

#include "state.h"
#include "input.h"
#include "mino.h"
#include "log.h"
#include "util.h"

static gameState* game = NULL; // Convenience pointer for storing app->game
static rng randomizer = {}; // RNG specifically for next piece selection

// Generate a new random piece for the player to control
static void newPiece(void) {
	game->player.x = PLAYFIELD_W/2 - PIECE_BOX/2; // Centered
	game->player.y = -2;
	game->player.type = random(&randomizer, PieceSize); // Naive, fully random randomizer
	if(game->player.type == PieceI)
		game->player.y += 1;
	game->player.rotation = 0;
}

// Check a single playfield cell
// Accepts inputs outside of bounds
static bool isFree(int x, int y) {
	if(x < 0 || x >= PLAYFIELD_W || y >= PLAYFIELD_H) return false;
	if(y < 0) return true;
	return (bool)(game->field[y][x] == MinoNone);
}

// Check that player's position doesn't overlap the playfield
static bool checkPosition(void) {
	for(int i = 0; i < MINOS_PER_PIECE; i++)
		if(!isFree(game->player.x + rs[game->player.type][game->player.rotation][i].x,
		           game->player.y + rs[game->player.type][game->player.rotation][i].y))
			return false;
	return true;
}

// Verify that player's position is legal, attempt kicks otherwise
// Returns true if successful, false if last move needs to be reverted
static bool tryKicks(void) {
	static int preference = 1;
	if(checkPosition()) return true; // Original position
	game->player.x += preference;
	if(checkPosition()) return true; // 1 to the right
	game->player.x -= 2*preference;
	if(checkPosition()) return true; // 1 to the left
	game->player.x += preference;
	return false; // Failure, returned to original position
}

// Attempt to move player piece sideways
// -1 is left
// 1 is right
static void shift(int direction) {
	game->player.x += direction;
	if(!checkPosition())
		game->player.x -= direction; // Revert on failure
}

// Attempt to rotate player piece
// 1 is CW
// -1 is CCW
static void rotate(int direction) {
	int prevRotation = game->player.rotation; // Keep track so that we don't have perform the annoying wrap twice
	game->player.rotation += direction;
	if(game->player.rotation >= 4) // Need to wrap manually because modulo is stupid below 0
		game->player.rotation -= 4;
	if(game->player.rotation < 0)
		game->player.rotation += 4;
	if(!tryKicks())
		game->player.rotation = prevRotation;
}

// "Unused function" warnings boo
/*static void sonicDrop(void) {
	while(checkPosition())
		game->player.y += 1;
	game->player.y -= 1;
}*/

// Stamp player piece into the playfield
// Does not do collision checking, so can overwrite filled cells
// Handles spawning of a new piece
static void lock(void) {
	for(int i = 0; i < MINOS_PER_PIECE; i++) {
		if(game->player.y + rs[game->player.type][game->player.rotation][i].y < 0) continue;
		game->field[game->player.y + rs[game->player.type][game->player.rotation][i].y]
		           [game->player.x + rs[game->player.type][game->player.rotation][i].x] = game->player.type;
	}
	newPiece();
}

// Maps generic inputs to gameplay commands
static cmdType inputToCmd(inputType i) {
	switch(i) {
		case InputLeft: return CmdLeft;
		case InputRight: return CmdRight;
		case InputUp: return CmdSonic;
		case InputDown: return CmdSoft;
		case InputButton1:                // This double mapping replicates the behavior of being unable
		case InputButton3: return CmdCCW; // to 180 if you press both on the same frame. It's also wrong, TODO
		case InputButton2: return CmdCW;
		default: return CmdNone;
	}
}

// Polls for inputs and transforms them into gameplay commands
static void processInputs(void) {
	input* i = NULL;
	while((i = dequeueInput())) {
		switch(i->action) {
			case ActionPressed:
				// Quitting is handled outside of gameplay logic
				if(i->type == InputQuit) {
					logInfo("User exited");
					setRunning(false);
					break;
				}
				
				// 4-way emulation
				if(i->type == InputUp || i->type == InputDown ||
				   i->type == InputLeft || i->type == InputRight) {
					game->cmdmap[inputToCmd(InputUp)] = false;
					game->cmdmap[inputToCmd(InputDown)] = false;
					game->cmdmap[inputToCmd(InputLeft)] = false;
					game->cmdmap[inputToCmd(InputRight)] = false;
				}
				
				if(inputToCmd(i->type) != CmdNone)
					game->cmdmap[inputToCmd(i->type)] = true;
				break;
				
			case ActionReleased:
				if(inputToCmd(i->type) != CmdNone)
					game->cmdmap[inputToCmd(i->type)] = false;
				break;
			default: break;
		}
		free(i);
	}
}

void initGameplay(void) {
	srandom(&randomizer, (uint64_t)time(NULL));
	
	game = allocate(sizeof(gameState));
	app->game = game;
	for(int y = 0; y < PLAYFIELD_H; y++)
	for(int x = 0; x < PLAYFIELD_W; x++)
		game->field[y][x] = MinoNone;
	game->dasDirection = 0;
	game->dasCharge = 0;
	game->dasDelay = DAS_DELAY; // Starts out pre-charged
	for(int i = 0; i < CmdSize; i++)
		game->cmdmap[i] = false;
	newPiece(); //TODO replace, this probably shouldn't be done here
}

void cleanupGameplay(void) {
	free(game);
	game = NULL;
	app->game = NULL;
}

void updateGameplay(void) {
	processInputs();
	
	// Check requested movement direction
	int shiftDirection = 0;
	if(game->cmdmap[CmdLeft])
		shiftDirection = -1;
	else if(game->cmdmap[CmdRight])
		shiftDirection = 1;
	
	// If not moving or moving in the opposite direction of ongoing DAS,
	// reset DAS and shift instantly
	if(shiftDirection == 0 || shiftDirection != game->dasDirection) {
		game->dasDirection = shiftDirection;
		game->dasCharge = 0;
		game->dasDelay = DAS_DELAY; // Starts out pre-charged
		if(shiftDirection) shift(shiftDirection);
	}
	
	// If moving, advance and apply DAS
	if(shiftDirection) {
		if(game->dasCharge < DAS_CHARGE) game->dasCharge += 1;
		if(game->dasCharge == DAS_CHARGE) {
			if(game->dasDelay < DAS_DELAY) game->dasDelay += 1;
			if(game->dasDelay == DAS_DELAY) {
				game->dasDelay = 0;
				shift(game->dasDirection);
			}
		}
	}
	
	// CW rotation
	if(game->cmdmap[CmdCW]) {
		rotate(1);
		game->cmdmap[CmdCW] = false;
	}
	
	// CCW rotation
	if(game->cmdmap[CmdCCW]) {
		rotate(-1);
		game->cmdmap[CmdCCW] = false;
	}
	
	// Piece locking (placeholder)
	if(game->cmdmap[CmdSoft]) {
		lock();
	}
}