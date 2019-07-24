#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "mino.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

typedef mino playfield[PLAYFIELD_H][PLAYFIELD_W];

typedef struct {
	int x, y;
	pieceType type;
	int rotation; // 0 to 3, 0 is spawn
} pieceState;

typedef struct {
	playfield field;
	pieceState player;
	bool rotCW, rotCCW;
} gameState;

void initGameplay(void);
void cleanupGameplay(void);
void updateGameplay(void);

#endif