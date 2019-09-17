// Minote - state.h
// Manages the logical state of the entire app

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "gameplay.h"
#include "replay.h"

enum state {
	StateNone,
	StateStaged,
	StateIntro,
	StateRunning,
	StatePaused,
	StateOutro,
	StateUnstaged,
	StateSize
};

enum phase {
	PhaseMain,
	PhaseGameplay,
	PhaseSize
};

struct app {
	struct game *game;
	struct replay *replay;
};

extern struct app *app; //SYNC appMutex
extern mutex appMutex;

void initState(void);
void cleanupState(void);

enum state getPhase(enum phase phase);
void setPhase(enum phase phase, enum state state);
#define isRunning() \
	(getPhase(PhaseMain) != StateNone)

#endif // STATE_H