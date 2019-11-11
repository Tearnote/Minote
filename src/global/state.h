// Minote - global/state.h
// Manages the state machine
// Gives thread-safe access to global state data

#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <stdbool.h>

#include "types/game.h"
#include "types/menu.h"
#include "util/thread.h"

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
	PhaseGame, // Primary
	PhaseSize
};

// List of global state structs
struct app {
	struct menu *menu;
	struct game *game;
};

void initState(void);
void cleanupState(void);

enum state getState(enum phase phase);
void setState(enum phase phase, enum state state);
// Candy function
bool isRunning(void);

// Return or write a copy of the global state structs
// for PhaseX, data type is struct x
void readStateData(enum phase phase, void *data);
void writeStateData(enum phase phase, void *data);

#endif //GLOBAL_STATE_H