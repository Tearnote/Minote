#include "state.h"

#include <stdbool.h>
#include <stdlib.h>

#include "thread.h"

state* game;
mutex stateMutex = newMutex;

void initState(void) {
	game = malloc(sizeof(state));
	game->running = true;
}

void cleanupState(void) {
	free(game);
}