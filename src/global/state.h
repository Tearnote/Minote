// Minote - state.h
// Manages the logical state of the entire app

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "util/thread.h"
#include "logic/menu.h"
#include "logic/gameplay.h"

enum state {
	StateNone,
	StateStaged,
	StateRunning,
	StateUnstaged,
	StateSize
};

enum phase {
	PhaseMain, // Meta, performs batch operations on other phases
	PhaseMenu, // Primary
	PhaseGameplay, // Primary
	PhaseSize
};

struct app {
	struct menu *menu;
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