// Minote - state.c

#include "state.h"

#include <stdlib.h>
#include <stdbool.h>

#include "util/thread.h"
#include "util/util.h"
#include "util/log.h"

enum state phases[PhaseSize];
mutex phaseMutex = newMutex;
struct app *app = NULL;
mutex appMutex = newMutex;

void initState(void)
{
	clearArray(phases);
	app = allocate(sizeof(*app));
	app->menu = allocate(sizeof(*app->menu));
	app->game = allocate(sizeof(*app->game));
	setState(PhaseMain, StateStaged);
}

void cleanupState(void)
{
	if (app->game)
		free(app->game);
	if (app->menu)
		free(app->menu);
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

bool isRunning(void)
{
	return getState(PhaseMain) != StateNone;
}

static void *phasePtr(enum phase phase)
{
	switch (phase) {
	case PhaseMenu:
		return app->menu;
	case PhaseGame:
		return app->game;
	default:
		logError("Queried for an invalid phase: %d", phase);
		return NULL;
	}
}

static size_t phasePtrSize(enum phase phase)
{
	switch (phase) {
	case PhaseMenu:
		return sizeof(*app->menu);
	case PhaseGame:
		return sizeof(*app->game);
	default:
		logError("Queried for an invalid phase: %d", phase);
		return 0;
	}
}

void readStateData(enum phase phase, void *data)
{
	void *ptr = phasePtr(phase);
	int ptrSize = phasePtrSize(phase);
	if (!ptr || ptrSize == 0)
		return;
	lockMutex(&appMutex);
	memcpy(data, ptr, ptrSize);
	unlockMutex(&appMutex);
}

void writeStateData(enum phase phase, void *data)
{
	void *ptr = phasePtr(phase);
	int ptrSize = phasePtrSize(phase);
	if (!ptr || ptrSize == 0)
		return;
	lockMutex(&appMutex);
	memcpy(ptr, data, ptrSize);
	unlockMutex(&appMutex);
}
