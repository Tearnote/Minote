// Minote - logic.c

#include "logic.h"

#include "gameplay.h"
#include "replay.h"
#include "thread.h"
#include "state.h"
#include "timer.h"

thread logicThreadID = 0;

double logicFrequency = DEFAULT_FREQUENCY;
#define LOGIC_TICK (SEC / logicFrequency)
static nsec nextUpdateTime = 0;

struct stateFunctions {
	void (*init)(void);
	void (*cleanup)(void);
	void (*update)(void);
};

struct stateFunctions funcs[AppSize] = {
	{ .init = noop, .cleanup = noop, .update = noop }, // AppNone
	{ .init = initGameplay, .cleanup = cleanupGameplay, .update = updateGameplay }, // AppGameplay
	{ .init = initReplay, .cleanup = cleanupReplay, .update = updateReplay } // AppReplay
};

static enum appState loadedState = AppNone;

static void updateLogic(void)
{
	if (!nextUpdateTime)
		nextUpdateTime = getTime();

	enum appState currentState = getState();
	if (loadedState != currentState) {
		lockMutex(&gameMutex);
		funcs[loadedState].cleanup();
		funcs[currentState].init();
		unlockMutex(&gameMutex);
		loadedState = currentState;
	}

	lockMutex(&gameMutex);
	funcs[loadedState].update();
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

	while (isRunning()) {
		updateLogic();
		sleepLogic();
	}

	lockMutex(&gameMutex);
	funcs[loadedState].cleanup();
	unlockMutex(&gameMutex);
	loadedState = AppNone;

	return NULL;
}