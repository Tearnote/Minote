// Minote - gameplay_pure.h
// Logic for the Pure gamemode

#ifndef PURE_H
#define PURE_H

#include <stdbool.h>

#include "types/game.h"
#include "gameplay.h"

void initGameplayPure(struct game *g);
void cleanupGameplayPure(struct game *g);

void advanceGameplayPure(struct game *g, bool cmd[GameCmdSize]);

#endif //PURE_H