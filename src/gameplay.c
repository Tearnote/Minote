#include "gameplay.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "state.h"
#include "input.h"
#include "mino.h"
#include "timer.h"
#include "log.h"
#include "util.h"

#define QUARTER_BEAT (120*MSEC)
#define JUDGMENT 4 // Size of the timing window as a fraction of the qbeat, higher is harder

static gameState* game;
static nsec gameTime = 0; // Clock of the game state
static nsec qbeatTime = 0; // Time of the next quarterbeat

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
		                       -4 + rs[game->player.type][game->player.rotation][i].y))
			return false;
	return true;
}

// Returns true if kick was successful, false if the move needs to be reverted
static bool tryKicks(void) {
	static int preference = 1;
	if(game->player.shifting != 0) preference = game->player.shifting;
	if(checkPosition()) return true;
	game->player.x += preference;
	if(checkPosition()) return true;
	game->player.x -= 2*preference;
	if(checkPosition()) return true;
	game->player.x += preference;
	return false;
}

static void shiftPlayerPiece(int direction) {
	game->player.x += direction;
	if(!checkPosition())
		game->player.x -= direction;
}

static void rotatePlayerPiece(int direction) {
	int prevRotation = game->player.rotation;
	game->player.rotation += direction;
	if(game->player.rotation >= 4)
		game->player.rotation -= 4;
	if(game->player.rotation < 0)
		game->player.rotation += 4;
	if(!tryKicks())
		game->player.rotation = prevRotation;
}

static void sonicDropPlayerPiece(void) {
	// Buckle up, we only got one shot at this.
}

static void lockPlayerPiece(void) {
	// How is function formed
}

void initGameplay(void) {
	game = allocate(1, sizeof(gameState));
	app->game = game;
	memcpy(game->field, (playfield){
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoT,    MinoNone, MinoNone, MinoNone },
		{ MinoO,    MinoO,    MinoZ,    MinoZ,    MinoNone, MinoT,    MinoL,    MinoL,    MinoNone, MinoNone },
		{ MinoO,    MinoO,    MinoT,    MinoT,    MinoNone, MinoNone, MinoNone, MinoL,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoNone, MinoNone, MinoT,    MinoL,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoS,    MinoT,    MinoT,    MinoT,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoS,    MinoS,    MinoNone, MinoZ,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoNone, MinoS,    MinoZ,    MinoZ,    MinoNone, MinoNone },
		{ MinoO,    MinoO,    MinoJ,    MinoJ,    MinoNone, MinoS,    MinoZ,    MinoJ,    MinoNone, MinoNone },
		{ MinoJ,    MinoJ,    MinoO,    MinoO,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoJ,    MinoJ,    MinoO,    MinoO,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone }
	}, sizeof(playfield));
	game->player.x = PLAYFIELD_W/2 - PIECE_BOX/2;
	game->player.type = PieceT;
	game->player.rotation = 0;
	game->player.shifting = 0;
	
	gameTime = qbeatTime = getTime();
}

void cleanupGameplay(void) {
	free(game);
	game = NULL;
	app->game = NULL;
}

void updateGameplay(nsec updateTime) {
	// These are static just in case we receive an input from the future and never process it
	static input* i = NULL;
	static nsec inputTime = -1; // Timestamp of received input
	
	while(true) { // Keep processing until we caught up to current time
		if(inputTime == -1) { // Check for new input if none waiting
			i = dequeueInput();
			if(i != NULL)
				inputTime = i->timestamp;
		}
		
		// Handle the input if it's the earliest event and not in the future
		if(inputTime != -1 && inputTime < qbeatTime && inputTime < updateTime) {
			if(i->type == InputBack && i->action == ActionPressed) {
				logInfo("User exited");
				setRunning(false);
			} else
			if(i->type == InputLeft && i->action == ActionPressed) {
				if(game->player.shifting == 0) { // If we're not shifting, accept the input but punish if too mistimed
					shiftPlayerPiece(-1);
					// Skip the next qbeat if the player hits ahead
					if(qbeatTime-inputTime < QUARTER_BEAT/JUDGMENT*(JUDGMENT-1))
						qbeatTime += QUARTER_BEAT;
				} else { // If already shifting, hitting too early is punished
					if(qbeatTime-inputTime < QUARTER_BEAT/JUDGMENT*(JUDGMENT-1) &&
					   qbeatTime-inputTime > QUARTER_BEAT/JUDGMENT)
						qbeatTime += QUARTER_BEAT;
				}
				game->player.shifting = -1;
			} else
			if(i->type == InputLeft && i->action == ActionReleased) {
				if(game->player.shifting == -1) game->player.shifting = 0; // Allow changing directions
			} else
			if(i->type == InputRight && i->action == ActionPressed) {
				if(game->player.shifting == 0) { // If we're not shifting, accept the input but punish if too mistimed
					shiftPlayerPiece(1);
					// Skip the next qbeat if the player hits ahead
					if(qbeatTime-inputTime < QUARTER_BEAT/JUDGMENT*(JUDGMENT-1))
						qbeatTime += QUARTER_BEAT;
				} else { // If already shifting, hitting too early is punished
					if(qbeatTime-inputTime < QUARTER_BEAT/JUDGMENT*(JUDGMENT-1) &&
					   qbeatTime-inputTime > QUARTER_BEAT/JUDGMENT)
						qbeatTime += QUARTER_BEAT;
				}
				game->player.shifting = 1;
			} else
			if(i->type == InputRight && i->action == ActionReleased) {
				if(game->player.shifting == 1) game->player.shifting = 0; // Allow changing directions
			} else
			if(i->type == InputRotCW && i->action == ActionPressed) {
				rotatePlayerPiece(1);
			} else
			if(i->type == InputRotCCW && i->action == ActionPressed) {
				rotatePlayerPiece(-1);
			} else
			if(i->type == InputSonic && i->action == ActionPressed) {
				sonicDropPlayerPiece();
			} else
			if(i->type == InputHard && i->action == ActionPressed) {
				if(game->player.shifting != 0 && // If we're just ahead of a qbeat, perform the shift ahead of time
				   qbeatTime-inputTime < QUARTER_BEAT/JUDGMENT) {
					shiftPlayerPiece(game->player.shifting);
					qbeatTime += QUARTER_BEAT;
				}
				sonicDropPlayerPiece();
				lockPlayerPiece();
			}
			
			gameTime = inputTime;
			free(i);
			i = NULL;
			inputTime = -1;
			continue;
		}
		
		// Handle the qbeat if it's not in the future (input is already handled if earliest)
		if(qbeatTime < updateTime) {
			if(game->player.shifting != 0) shiftPlayerPiece(game->player.shifting);
			
			gameTime = qbeatTime;
			qbeatTime += QUARTER_BEAT;
			continue;
		}
		
		// All events handled or in the future, catch up and leave
		gameTime = updateTime;
		break;
	}
}