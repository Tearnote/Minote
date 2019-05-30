#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "mino.h"
#include "timer.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

typedef struct {
	int x;
	pieceType type;
	int rotation; // 0 to 3, 0 is spawn
} controlledPiece;

typedef struct {
	mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	controlledPiece playerPiece;
	int shifting; // -1 for left, 0 for no, 1 for right
} state;

extern bool running;
extern mutex runningMutex;
extern state* game;
extern mutex gameMutex;

void initState(void);
void cleanupState(void);
#define isRunning() syncBoolRead(&running, &runningMutex)
#define setRunning(x) syncBoolWrite(&running, (x), &runningMutex)

#endif