// Minote - state.c

#include "state.h"

#include <stdlib.h>

#include "thread.h"
#include "util.h"

enum state phases[PhaseSize];
mutex phaseMutex = newMutex;
struct app *app = NULL;
mutex appMutex = newMutex;

void initState(void)
{
	clearArray(phases);
	setState(PhaseMain, StateStaged);
	app = allocate(sizeof(*app));
	app->game = allocate(sizeof(*app->game));
}

void cleanupState(void)
{
	if (app->game)
		free(app->game);
	if (app) {
		free(app);
		app = NULL;
	}
}

enum state getState(enum phase phase)
{
	lockMutex(&phaseMutex);
	enum state result = phases[phase];
	unlockMutex(&phaseMutex);
	return result;
}

void setState(enum phase phase, enum state state)
{
	lockMutex(&phaseMutex);
	phases[phase] = state;
	unlockMutex(&phaseMutex);
}