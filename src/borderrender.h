// Minote - linerender.h
// Renders the border around the playfield

#ifndef BORDERRENDER_H
#define BORDERRENDER_H

#include "mino.h"
#include "gameplay.h"

void initBorderRenderer(void);
void cleanupBorderRenderer(void);

// Add border segments to the queue
void queueBorder(enum mino playfield[PLAYFIELD_H][PLAYFIELD_W]);

// Render all border segments in one go, clear the queue
void renderBorder(void);

#endif //BORDERRENDER_H
