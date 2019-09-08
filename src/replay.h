// Minote - replay.h
// Keeps replays, loads and saves them, helps play them back

#ifndef REPLAY_H
#define REPLAY_H

#include "gameplay.h"

void initReplay(void);
void cleanupReplay(void);

void pushState(struct game *state);

#endif //REPLAY_H
