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
static nsec nextUpdateTime = 0; // Goal for the next update

static void updateLogic(void) {
	if(!nextUpdateTime) nextUpdateTime = getTime();
	lockMutex(&gameMutex);
	updateGameplay();
	unlockMutex(&gameMutex);
}

static void sleepLogic(void) {
	nextUpdateTime += LOGIC_TICK;
	nsec timeRemaining = nextUpdateTime - getTime();
	if(timeRemaining > 0)
		sleep(timeRemaining);
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