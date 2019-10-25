// Minote - logic.c

#include "logic.h"

#include "gameplay.h"
#include "thread.h"
#include "state.h"
#include "timer.h"

thread logicThreadID = 0;

atomic double logicFrequency = DEFAULT_FREQUENCY;
#define LOGIC_TICK (SEC / logicFrequency)
static nsec nextUpdateTime = -1;

static void updateLogic(void)
{
	if (nextUpdateTime == -1)
		nextUpdateTime = getTime();

	switch (getState(PhaseMain)) {
	case StateStaged:
		setState(PhaseMain, StateRunning);
		setState(PhaseGameplay, StateStaged);
		break;
	case StateUnstaged:
		if (getState(PhaseGameplay) != StateNone) {
			setState(PhaseGameplay, StateUnstaged);
		}
		setState(PhaseMain, StateNone);
	default:
		break;
	}

	switch (getState(PhaseGameplay)) {
	case StateStaged:
		initGameplay();
		// "break;" missing on purpose, no point in wasting a frame
	case StateIntro:
	case StateRunning:
	case StateOutro:
		updateGameplay();
		break;
	case StateUnstaged:
		cleanupGameplay();
		break;
	default:
		break;
	}
}

static void sleepLogic(void)
{
	nextUpdateTime += LOGIC_TICK;
	sleep(nextUpdateTime);
}

void *logicThread(void *param)
{
	(void)param;

	while (isRunning()) {
		updateLogic();
		sleepLogic();
	}
	return NULL;
}