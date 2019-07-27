// Minote - gameplay.h
// Holds and handles gameplay logic

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "mino.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

typedef mino playfield[PLAYFIELD_H][PLAYFIELD_W];

// Types of commands accepted by the gameplay
typedef enum {
	CmdNone,
	CmdLeft, CmdRight,
	CmdCW, CmdCCW,
	CmdSoft, CmdSonic,
	CmdSize
} cmdType;

// Description of a tetromino on a playfield
typedef struct {
	int x, y;
	pieceType type;
	int rotation; // 0 to 3, 0 is spawn
} pieceState;

// Complete description of the gameplay's current state
// Does not use pointers, so that it can be copied and serialized
typedef struct {
	playfield field;
	pieceState player;
	bool cmdmap[CmdSize];
	int dasDirection, dasCharge, dasDelay;
} gameState;

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

#endif