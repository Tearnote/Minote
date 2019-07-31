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

// Check whether player piece can drop one grid
static bool canDrop(void) {
	game->player.y += 1;
	bool result = checkPosition();
	game->player.y -= 1;
	return result;
}

static void drop(void) {
	if(canDrop()) {
		game->player.y += 1;
		game->player.lockDelay = 0;
	}
}

// Stamp player piece into the playfield
// Does not do collision checking, so can overwrite filled cells
// Handles spawning of a new piece
static void lock(void) {
	for(int i = 0; i < MINOS_PER_PIECE; i++) {
		if(game->player.y + rs[game->player.type][game->player.rotation][i].y < 0) continue;
		game->field[game->player.y + rs[game->player.type][game->player.rotation][i].y]
		           [game->player.x + rs[game->player.type][game->player.rotation][i].x] = game->player.type;
	}
	game->player.exists = false;
}

// Generate a new random piece for the player to control
static void newPiece(void) {
	game->player.exists = true;
	game->player.x = PLAYFIELD_W/2 - PIECE_BOX/2; // Centered
	game->player.y = -2;
	
	// Picking the next piece
	if(game->player.preview == PieceNone)
		game->player.preview = random(&randomizer, PieceSize-1) + 1;
	game->player.type = game->player.preview;
	game->player.preview = random(&randomizer, PieceSize-1) + 1;
	
	if(game->player.type == PieceI)
		game->player.y += 1;
	game->player.ySub = 0;
	game->player.lockDelay = 0;
	game->player.spawnDelay = 0;
	game->player.rotation = 0;
	
	// IRS
	if(game->cmdHeld[CmdCW])
		rotate(1);
	else if(game->cmdHeld[CmdCCW] || game->cmdHeld[CmdCCW2])
		rotate(-1);
}

// Maps generic inputs to gameplay commands
static cmdType inputToCmd(inputType i) {
	switch(i) {
		case InputLeft: return CmdLeft;
		case InputRight: return CmdRight;
		case InputUp: return CmdSonic;
		case InputDown: return CmdSoft;
		case InputButton1: return CmdCCW;
		case InputButton2: return CmdCW;
		case InputButton3: return CmdCCW2;
		default: return CmdNone;
	}
}

// Polls for inputs and transforms them into gameplay commands
static void processInputs(void) {
	// Clear previous frame's presses
	for(int i = 0; i < CmdSize; i++)
		game->cmdPressed[i] = false;
	
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
					game->cmdPressed[inputToCmd(InputUp)] = false;
					game->cmdPressed[inputToCmd(InputDown)] = false;
					game->cmdPressed[inputToCmd(InputLeft)] = false;
					game->cmdPressed[inputToCmd(InputRight)] = false;
					game->cmdHeld[inputToCmd(InputUp)] = false;
					game->cmdHeld[inputToCmd(InputDown)] = false;
					game->cmdHeld[inputToCmd(InputLeft)] = false;
					game->cmdHeld[inputToCmd(InputRight)] = false;
				}
				
				if(inputToCmd(i->type) != CmdNone) {
					game->cmdPressed[inputToCmd(i->type)] = true;
					game->cmdHeld[inputToCmd(i->type)] = true;
				}
				break;
				
			case ActionReleased:
				if(inputToCmd(i->type) != CmdNone) {
					game->cmdPressed[inputToCmd(i->type)] = false;
					game->cmdHeld[inputToCmd(i->type)] = false;
				}
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
	for(int i = 0; i < CmdSize; i++) {
		game->cmdPressed[i] = false;
		game->cmdHeld[i] = false;
	}
	game->player.exists = false;
	game->player.x = 0;
	game->player.y = 0;
	game->player.ySub = 0;
	game->player.type = PieceNone;
	game->player.preview = PieceNone;
	game->player.rotation = 0;
	game->player.dasDirection = 0;
	game->player.dasCharge = 0;
	game->player.dasDelay = DAS_DELAY; // Starts out pre-charged
	game->player.lockDelay = 0;
	game->player.spawnDelay = SPAWN_DELAY-1; // Start instantly
}

void cleanupGameplay(void) {
	free(game);
	game = NULL;
	app->game = NULL;
}

void updateGameplay(void) {
	processInputs();
	
	if(game->player.exists) {
		// CW rotation
		if(game->cmdPressed[CmdCW])
			rotate(1);
		
		// CCW rotation
		if(game->cmdPressed[CmdCCW] || game->cmdPressed[CmdCCW2])
			rotate(-1);
	}
	
	// Check requested movement direction
	int shiftDirection = 0;
	if(game->cmdHeld[CmdLeft])
		shiftDirection = -1;
	else if(game->cmdHeld[CmdRight])
		shiftDirection = 1;
	
	// If not moving or moving in the opposite direction of ongoing DAS,
	// reset DAS and shift instantly
	if(shiftDirection == 0 || shiftDirection != game->player.dasDirection) {
		game->player.dasDirection = shiftDirection;
		game->player.dasCharge = 0;
		game->player.dasDelay = DAS_DELAY; // Starts out pre-charged
		if(shiftDirection && game->player.exists) shift(shiftDirection);
	}
	
	// If moving, advance and apply DAS
	if(shiftDirection) {
		if(game->player.dasCharge < DAS_CHARGE) game->player.dasCharge += 1;
		if(game->player.dasCharge == DAS_CHARGE) {
			if(game->player.dasDelay < DAS_DELAY && game->player.exists) game->player.dasDelay += 1;
			if(game->player.dasDelay == DAS_DELAY && game->player.exists) { // If during ARE, keep the DAS charged
				game->player.dasDelay = 0;
				shift(game->player.dasDirection);
			}
		}
	}
	
	// Calculate this frame's gravity speed
	int gravity = GRAVITY;
	if(game->player.exists) {
		if(game->cmdHeld[CmdSoft] && gravity < SOFT_DROP)
			gravity = SOFT_DROP;
		if(game->cmdPressed[CmdSonic])
			gravity = SONIC_DROP;
	}
	
	// Manlock
	if(game->player.exists && game->cmdHeld[CmdSoft] && !canDrop())
		lock();
	
	// Handle ARE and spawning
	if(!game->player.exists) {
		game->player.spawnDelay += 1;
		if(game->player.spawnDelay == SPAWN_DELAY)
			newPiece();
	}
	
	if(game->player.exists) {
		// Apply gravity
		game->player.ySub += gravity;
		while(game->player.ySub >= SUBGRID) {
			drop();
			game->player.ySub -= SUBGRID;
		}
		
		// Advance+apply lock delay
		if(!canDrop()) {
			game->player.lockDelay += 1;
			if(game->player.lockDelay == LOCK_DELAY)
				lock();
		}
	}
}