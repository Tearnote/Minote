// Minote - state.c

#include "state.h"

#include <stdlib.h>

#include "thread.h"
#include "util.h"

enum state phases[PhaseSize];
mutex phaseMutex = newMutex;
struct app *app;
mutex appMutex = newMutex;

void initState(void)
{
	clearArray(phases);
	setPhase(PhaseMain, StateStaged);
	app = allocate(sizeof(*app));
	app->game = allocate(sizeof(*app->game));
	app->replay = allocate(sizeof(*app->replay));
}

void cleanupState(void)
{
	if (app->game)
		free(app->game);
	if (app->replay)
		free(app->replay);
	if (app) {
		free(app);
		app = NULL;
	}
}

enum state getPhase(enum phase phase)
{
	lockMutex(&phaseMutex);
	enum state result = phases[phase];
	unlockMutex(&phaseMutex);
	return result;
}

void setPhase(enum phase phase, enum state state)
{
	lockMutex(&phaseMutex);
	phases[phase] = state;
	unlockMutex(&phaseMutex);
}