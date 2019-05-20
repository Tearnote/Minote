#ifndef MINORENDER_H
#define MINORENDER_H

#include "mino.h"

void initMinoRenderer(void);
void cleanupMinoRenderer(void);
void queueMinoPlayfield(void);
void renderMino(void);

#endif