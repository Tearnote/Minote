// Minote - gameplay_pure.h
// Logic for the Pure gamemode

#ifndef GAMEPLAY_PURE_H
#define GAMEPLAY_PURE_H

#include "gameplay.h"

void initGameplayPure(struct game *g);
void cleanupGameplayPure(struct game *g);

void advanceGameplayPure(struct game *g, bool cmd[GameCmdSize]);

#endif // GAMEPLAY_PURE_H