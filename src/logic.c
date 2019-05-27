#include "logic.h"

#include <stdbool.h>

#include "thread.h"
#include "state.h"
#include "timer.h"
#include "log.h"
#include "input.h"

#define LOGIC_FREQUENCY 1000 // in Hz
#define TIME_PER_UPDATE (SEC / LOGIC_FREQUENCY)
static nsec lastUpdateTime = 0;

thread logicThreadID = 0;

static void updateLogic(void) {
	lastUpdateTime = getTime();
	
	lockMutex(&stateMutex);
	controlledPiece* ppiece = &game->playerPiece; // Bad variable name I know, but less of a mouthful
	
	for(input* i = dequeueInput(); !!i; i = dequeueInput()) {
		if(i->type == InputBack && i->action == ActionPressed) {
				logInfo("User exited");
				game->running = false; // Don't be me and lock a mutex inside a mutex...
		} else
		if(i->type == InputLeft && i->action == ActionPressed) {
			ppiece->x -= 1;
		} else
		if(i->type == InputRight && i->action == ActionPressed) {
			ppiece->x += 1;
		} else
		if(i->type == InputRotCW && i->action == ActionPressed) {
			ppiece->rotation += 1;
			ppiece->rotation %= 4;
		} else
		if(i->type == InputRotCCW && i->action == ActionPressed) {
			ppiece->rotation -= 1;
			if(ppiece->rotation < 0) // Shame on C for % being remainder, not modulo
				ppiece->rotation += 4;
		}
		
		// Wallstop and wallkick
		if(ppiece->x + rightmostMino(rs[ppiece->type][ppiece->rotation]) >= PLAYFIELD_W-1) {
			ppiece->x = PLAYFIELD_W-1 - rightmostMino(rs[ppiece->type][ppiece->rotation]);
		} else
		if(ppiece->x + leftmostMino(rs[ppiece->type][ppiece->rotation]) < 0) {
			ppiece->x = -leftmostMino(rs[ppiece->type][ppiece->rotation]);
		}
		
		free(i);
	}
	unlockMutex(&stateMutex);
}

static void sleepLogic(void) {
	nsec timePassed = getTime() - lastUpdateTime;
	if(timePassed < TIME_PER_UPDATE) // Only bother sleeping if we're ahead of the target
		sleep(TIME_PER_UPDATE - timePassed);
}

void* logicThread(void* param) {
	(void)param;
	
	while(isRunning()) {
		updateLogic();
		sleepLogic();
	}
	
	return NULL;
}