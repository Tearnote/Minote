// Minote - logic/pure.h
// Logic for the Pure gamemode

#ifndef LOGIC_PURE_H
#define LOGIC_PURE_H

#include <stdbool.h>

#include "types/game.h"

void initGameplayPure(struct game *g);
void cleanupGameplayPure(struct game *g);

void advanceGameplayPure(struct game *g, bool cmd[GameCmdSize]);

#endif //LOGIC_PURE_H