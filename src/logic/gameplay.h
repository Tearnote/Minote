// Minote - logic/gameplay.h
// Handles gameplay updates

#ifndef LOGIC_GAMEPLAY_H
#define LOGIC_GAMEPLAY_H

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

#endif //LOGIC_GAMEPLAY_H