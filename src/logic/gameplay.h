// Minote - gameplay.h
// Handles gameplay logic

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "types/mino.h"
#include "types/game.h"
#include "util/timer.h"
#include "util/util.h"

// Define the length of a frame for the purpose of calculating the timer
// This is not equal to real time
#define TIMER_FRAMERATE 60 // in Hz
#define TIMER_FRAME (SEC / TIMER_FRAMERATE)

// Subgrid value at which the piece is dropped
#define SUBGRID 256

// The number of times the randomizer attempts to pick a piece not in history
#define MAX_REROLLS 4

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

// Return the mino at the specific cell
// Accepts inputs outside of bounds
enum mino
getPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W], int x, int y);
void setPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W],
                      int x, int y, enum mino val);

#endif // GAMEPLAY_H
