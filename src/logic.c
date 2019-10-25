// Minote - logic.c

#include "logic.h"

#include "menu.h"
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
		setState(PhaseMenu, StateStaged);
		break;
	case StateUnstaged:
		// Abort starting new things, clean up started things
		if (getState(PhaseMenu) == StateStaged)
			setState(PhaseMenu, StateNone);
		else if (getState(PhaseMenu) == StateRunning)
			setState(PhaseMenu, StateUnstaged);
		if (getState(PhaseGameplay) == StateStaged)
			setState(PhaseGameplay, StateNone);
		else if (getState(PhaseGameplay) == StateRunning)
			setState(PhaseGameplay, StateUnstaged);
		setState(PhaseMain, StateNone);
	default:
		break;
	}

	switch (getState(PhaseMenu)) {
	case StateStaged:
		initMenu();
		// break;
	case StateRunning:
		updateMenu();
		break;
	case StateUnstaged:
		cleanupMenu();
		break;
	default:
		break;
	}

	switch (getState(PhaseGameplay)) {
	case StateStaged:
		initGameplay();
		// break;
	case StateRunning:
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