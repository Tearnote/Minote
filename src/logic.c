// Minote - logic.c

#include "logic.h"

#include "thread.h"
#include "state.h"
#include "timer.h"

thread logicThreadID = 0;

#define LOGIC_FREQUENCY 59.84 // in Hz
#define LOGIC_TICK (SEC / LOGIC_FREQUENCY)
static nsec nextUpdateTime = 0;

static enum appState loadedState = AppNone;

static void updateLogic(void)
{
	if (!nextUpdateTime)
		nextUpdateTime = getTime();
	lockMutex(&gameMutex);
	updateGameplay();
	unlockMutex(&gameMutex);
}

static void sleepLogic(void)
{
	nextUpdateTime += LOGIC_TICK;
	nsec timeRemaining = nextUpdateTime - getTime();
	if (timeRemaining > 0)
		sleep(timeRemaining);
}

void *logicThread(void *param)
{
	(void)param;

	lockMutex(&gameMutex);
	initGameplay();
	unlockMutex(&gameMutex);

	while (isRunning()) {
		updateLogic();
		sleepLogic();
	}

	lockMutex(&gameMutex);
	cleanupGameplay();
	unlockMutex(&gameMutex);

	return NULL;
}
