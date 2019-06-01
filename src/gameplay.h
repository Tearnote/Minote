#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "mino.h"
#include "timer.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

typedef mino playfield[PLAYFIELD_H][PLAYFIELD_W];

void initGameplay(void);
void updateGameplay(nsec updateTime);

#endif