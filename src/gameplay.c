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

#define DAS_CHARGE 16
#define DAS_DELAY 1 // 0 not supported yet

static gameState* game;
static rng randomizer = {};

static bool inputSnap[InputSize] = {};

static void newPiece(void) {
	game->player.x = PLAYFIELD_W/2 - PIECE_BOX/2;
	game->player.y = -4;
	game->player.type = random(&randomizer, PieceSize);
	game->player.rotation = 0;
}

// Accepts inputs outside of bounds
static bool isFree(int x, int y) {
	if(x < 0 || x >= PLAYFIELD_W || y >= PLAYFIELD_H) return false;
	if(y < 0) return true;
	return (bool)(game->field[y][x] == MinoNone);
}

// Returns true if current position doesn't overlap the playfield
static bool checkPosition(void) {
	for(int i = 0; i < MINOS_PER_PIECE; i++)
		if(!isFree(game->player.x + rs[game->player.type][game->player.rotation][i].x,
		           game->player.y + rs[game->player.type][game->player.rotation][i].y))
			return false;
	return true;
}

// Returns true if kick was successful, false if the move needs to be reverted
static bool tryKicks(void) {
	static int preference = 1;
	if(checkPosition()) return true;
	game->player.x += preference;
	if(checkPosition()) return true;
	game->player.x -= 2*preference;
	if(checkPosition()) return true;
	game->player.x += preference;
	return false;
}

static void shift(int direction) {
	game->player.x += direction;
	if(!checkPosition())
		game->player.x -= direction;
}

static void rotate(int direction) {
	int prevRotation = game->player.rotation;
	game->player.rotation += direction;
	if(game->player.rotation >= 4)
		game->player.rotation -= 4;
	if(game->player.rotation < 0)
		game->player.rotation += 4;
	if(!tryKicks())
		game->player.rotation = prevRotation;
}

/*static void sonicDrop(void) {
	while(checkPosition())
		game->player.y += 1;
	game->player.y -= 1;
}*/

/*static void lock(void) {
	for(int i = 0; i < MINOS_PER_PIECE; i++) {
		if(game->player.y + rs[game->player.type][game->player.rotation][i].y < 0) continue;
		game->field[game->player.y + rs[game->player.type][game->player.rotation][i].y]
		           [game->player.x + rs[game->player.type][game->player.rotation][i].x] = game->player.type;
	}
	newPiece();
}*/

void initGameplay(void) {
	srandom(&randomizer, (uint64_t)time(NULL));
	
	game = allocate(sizeof(gameState));
	app->game = game;
	for(int y = 0; y < PLAYFIELD_H; y++)
	for(int x = 0; x < PLAYFIELD_W; x++)
		game->field[y][x] = MinoNone;
	game->dasDirection = 0;
	game->dasCharge = 0;
	game->dasDelay = DAS_DELAY;
	game->rotCW = false;
	game->rotCCW = false;
	newPiece();
}

void cleanupGameplay(void) {
	free(game);
	game = NULL;
	app->game = NULL;
}

void updateGameplay(void) {
	lockMutex(&inputMutex);
	memcpy(inputSnap, inputs, sizeof(inputs));
	unlockMutex(&inputMutex);
	
	int shiftDirection = 0;
	
	if(inputSnap[InputLeft])
		shiftDirection = -1;
	else if(inputSnap[InputRight])
		shiftDirection = 1;
	
	if(shiftDirection == 0 || shiftDirection != game->dasDirection) {
		game->dasDirection = shiftDirection;
		game->dasCharge = 0;
		game->dasDelay = DAS_DELAY;
		if(shiftDirection) shift(shiftDirection);
	}
	
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
	
	if(inputSnap[InputRotCW] && !game->rotCW) {
		rotate(1);
		game->rotCW = true;
	}
	if(!inputSnap[InputRotCW]){
		game->rotCW = false;
	}
	if(inputSnap[InputRotCCW] && !game->rotCCW) {
		rotate(-1);
		game->rotCCW = true;
	}
	if(!inputSnap[InputRotCCW]) {
		game->rotCCW = false;
	}
	
	if(inputSnap[InputBack]) {
		logInfo("User exited");
		setRunning(false);
	}
}