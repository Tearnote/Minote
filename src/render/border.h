// Minote - linerender.h
// Renders the border around the playfield

#ifndef BORDER_H
#define BORDER_H

#include "types/mino.h"
#include "logic/gameplay.h"

void initBorderRenderer(void);
void cleanupBorderRenderer(void);

// Add border segments to the darray
void queueBorder(enum mino playfield[PLAYFIELD_H][PLAYFIELD_W]);

// Render all border segments in one go, clear the darray
void renderBorder(void);

#endif //BORDER_H
