#include "logic.h"

#include <stdbool.h>

#include "thread.h"
#include "state.h"
#include "timer.h"
#include "log.h"
#include "input.h"

thread logicThreadID = 0;

#define LOGIC_FREQUENCY 1000 // in Hz
#define TIME_PER_UPDATE (SEC / LOGIC_FREQUENCY)
static nsec updateTime = 0; // This update's timestamp

#define QUARTER_BEAT (108*MSEC)
#define JUDGMENT 4 // Size of the timing window as a fraction of the qbeat
static nsec logicTime = 0; // Clock of the game state
static nsec qbeatTime = 0; // Time of the next quarterbeat

static void checkKicks(void) {
	controlledPiece* ppiece = &game->playerPiece; // Bad variable name I know, but less of a mouthful
	
	// Wallstop / wallkick
	if(ppiece->x + rightmostMino(rs[ppiece->type][ppiece->rotation]) >= PLAYFIELD_W-1) {
		ppiece->x = PLAYFIELD_W-1 - rightmostMino(rs[ppiece->type][ppiece->rotation]);
	} else
	if(ppiece->x + leftmostMino(rs[ppiece->type][ppiece->rotation]) < 0) {
		ppiece->x = -leftmostMino(rs[ppiece->type][ppiece->rotation]);
	}
}

static void shiftPlayerPiece(int direction) {
	game->playerPiece.x += direction;
	checkKicks();
}

static void rotatePlayerPiece(int direction) {
	game->playerPiece.rotation += direction;
	if(game->playerPiece.rotation >= 4)
		game->playerPiece.rotation -= 4;
	if(game->playerPiece.rotation < 0)
		game->playerPiece.rotation += 4;
	checkKicks();
}

static void updateLogic(void) {
	updateTime = getTime();
	
	lockMutex(&stateMutex);
	
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
				game->running = false; // Don't be me and lock a mutex inside a mutex...
			} else
			if(i->type == InputLeft && i->action == ActionPressed) {
				if(game->shifting == 0) // Don't shift if changing directions
					shiftPlayerPiece(-1);
				game->shifting = -1;
				// Allow the player to hit slightly ahead
				if(qbeatTime > inputTime && (qbeatTime - inputTime) < QUARTER_BEAT/JUDGMENT)
					qbeatTime += QUARTER_BEAT;
			} else
			if(i->type == InputLeft && i->action == ActionReleased) {
				if(game->shifting == -1) game->shifting = 0; // Allow changing directions
			} else
			if(i->type == InputRight && i->action == ActionPressed) {
				if(game->shifting == 0) // Don't shift if changing directions
					shiftPlayerPiece(1);
				game->shifting = 1;
				// Allow the player to hit slightly ahead
				if(qbeatTime > inputTime && (qbeatTime - inputTime) < QUARTER_BEAT/JUDGMENT)
					qbeatTime += QUARTER_BEAT;
			} else
			if(i->type == InputRight && i->action == ActionReleased) {
				if(game->shifting == 1) game->shifting = 0; // Allow changing directions
			} else
			if(i->type == InputRotCW && i->action == ActionPressed) {
				rotatePlayerPiece(1);
			} else
			if(i->type == InputRotCCW && i->action == ActionPressed) {
				rotatePlayerPiece(-1);
			}
			
			logicTime = inputTime;
			free(i);
			i = NULL;
			inputTime = -1;
			continue;
		}
		
		// Handle the qbeat if it's not in the future (input is already handled if earliest)
		if(qbeatTime < updateTime) {
			if(game->shifting != 0) shiftPlayerPiece(game->shifting);
			
			logicTime = qbeatTime;
			qbeatTime += QUARTER_BEAT;
			continue;
		}
		
		// All events handled or in the future, catch up and leave
		logicTime = updateTime;
		break;
	}
	
	unlockMutex(&stateMutex);
}

static void sleepLogic(void) {
	nsec timePassed = getTime() - updateTime;
	if(timePassed < TIME_PER_UPDATE) // Only bother sleeping if we're ahead of the target
		sleep(TIME_PER_UPDATE - timePassed);
}

void* logicThread(void* param) {
	(void)param;
	
	logicTime = qbeatTime = getTime();
	
	while(isRunning()) {
		updateLogic();
		sleepLogic();
	}
	
	return NULL;
}