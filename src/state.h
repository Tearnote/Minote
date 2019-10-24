// Minote - state.h
// Manages the logical state of the entire app

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "gameplay.h"

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
	PhaseMain, // Meta, performs batch operations on other phases
	PhaseGameplay, // Primary
	PhaseSize
};

struct app {
	struct game *game;
};

extern struct app *app; //SYNC appMutex
extern mutex appMutex;

void initState(void);
void cleanupState(void);

enum state getState(enum phase phase);
void setState(enum phase phase, enum state state);
#define isRunning() \
	(getState(PhaseMain) != StateNone)

#endif // STATE_H