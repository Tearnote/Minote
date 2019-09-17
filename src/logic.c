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

static void updateLogic(void)
{
	if (!nextUpdateTime)
		nextUpdateTime = getTime();

	switch (getPhase(PhaseMain)) {
	case StateStaged:
		setPhase(PhaseMain, StateRunning);
		setPhase(PhaseGameplay, StateStaged);
		break;
	case StateUnstaged:
		if (getPhase(PhaseGameplay) != StateNone) {
			setPhase(PhaseGameplay, StateUnstaged);
		}
		setPhase(PhaseMain, StateNone);
	default:
		break;
	}

	switch (getPhase(PhaseGameplay)) {
	case StateStaged:
		initGameplay();
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
	return NULL;
}