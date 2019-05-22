#ifndef MINORENDER_H
#define MINORENDER_H

#include "mino.h"
#include "state.h"

void initMinoRenderer(void);
void cleanupMinoRenderer(void);
void queueMinoPlayfield(mino playfield[][PLAYFIELD_W]);
void renderMino(void);

#endif