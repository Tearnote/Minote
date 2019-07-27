// Minote - logic.c

#include "logic.h"

#include <stdbool.h>

#include "thread.h"
#include "state.h"
#include "timer.h"
#include "log.h"
#include "input.h"

thread logicThreadID = 0;

#define LOGIC_FREQUENCY 60 // in Hz
#define LOGIC_TICK (SEC / LOGIC_FREQUENCY)
static nsec updateTime = 0; // This update's timestamp

static void updateLogic(void) {
	updateTime = getTime();
	lockMutex(&gameMutex);
	updateGameplay();
	unlockMutex(&gameMutex);
}

static void sleepLogic(void) {
	nsec timePassed = getTime() - updateTime;
	if(timePassed < LOGIC_TICK) // Only bother sleeping if we're ahead of the target
		sleep(LOGIC_TICK - timePassed);
}

void* logicThread(void* param) {
	(void)param;
	
	lockMutex(&gameMutex);
	initGameplay();
	unlockMutex(&gameMutex);
	
	while(isRunning()) {
		updateLogic();
		sleepLogic();
	}
	
	lockMutex(&gameMutex);
	cleanupGameplay();
	unlockMutex(&gameMutex);
	
	return NULL;
}