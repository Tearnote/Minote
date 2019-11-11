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

// Frames until the first autoshift
// 2 is right after the normal shift
#define DAS_CHARGE 16

// Number of frames between autoshifts
// 1 is every frame
// 0 is instant (not supported yet)
#define DAS_DELAY 1

// Subgrid value at which the piece is dropped
#define SUBGRID 256

// Natural piece falling speed in subgrids
extern int GRAVITY;

// Piece falling speed if soft drop is held
#define SOFT_DROP 256

// Piece falling speed if sonic drop is held
#define SONIC_DROP 5120

// How many frames a piece takes to lock if it can't drop
#define LOCK_DELAY 30

// How many frames it takes since locking for the line clear check
#define CLEAR_OFFSET 4

// How many frames it takes for full lines to clear
#define CLEAR_DELAY 41

// How many frames it takes for the next piece to spawn
// after the previous one is locked
#define SPAWN_DELAY 30

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
