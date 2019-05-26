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
	for(input* i = dequeueInput(); !!i; i = dequeueInput()) {
		if(i->type == InputBack && i->action == ActionPressed) {
				logInfo("User exited");
				setRunning(false);
		} else
		if(i->type == InputLeft && i->action == ActionPressed) {
			game->playerPiece.x -= 1;
		} else
		if(i->type == InputRight && i->action == ActionPressed) {
			game->playerPiece.x += 1;
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