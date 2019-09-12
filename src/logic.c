// Minote - logic.c

#include "logic.h"

#include "gameplay.h"
#include "replay.h"
#include "thread.h"
#include "state.h"
#include "timer.h"

thread logicThreadID = 0;

#define LOGIC_FREQUENCY 59.84 // in Hz
#define LOGIC_TICK (SEC / LOGIC_FREQUENCY)
static nsec nextUpdateTime = 0;

struct stateFunctions {
	void (*init)(void);
	void (*cleanup)(void);
	void (*update)(void);
	mutex *lock;
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
		lockMutex(funcs[loadedState].lock);
		funcs[loadedState].cleanup();
		unlockMutex(funcs[loadedState].lock);
		lockMutex(funcs[currentState].lock);
		funcs[currentState].init();
		unlockMutex(funcs[currentState].lock);
		loadedState = currentState;
	}

	lockMutex(funcs[loadedState].lock);
	funcs[loadedState].update();
	unlockMutex(funcs[loadedState].lock);
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

	funcs[AppNone].lock = NULL;
	funcs[AppGameplay].lock = &gameMutex;
	funcs[AppReplay].lock = &replayMutex;

	while (isRunning()) {
		updateLogic();
		sleepLogic();
	}

	lockMutex(funcs[loadedState].lock);
	funcs[loadedState].cleanup();
	unlockMutex(funcs[loadedState].lock);
	loadedState = AppNone;

	return NULL;
}
