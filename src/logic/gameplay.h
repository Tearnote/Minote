// Minote - logic/gameplay.h
// Handles gameplay updates

#ifndef LOGIC_GAMEPLAY_H
#define LOGIC_GAMEPLAY_H

#include "util/timer.h"

// The length of a frame for the purpose of calculating the timer
// Emulates time drift
#define TIMER_FRAMERATE 60 // in Hz
#define TIMER_FRAME (SEC / TIMER_FRAMERATE)

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

#endif // LOGIC_GAMEPLAY_H