// Minote - render/border.h
// Renders the border around the playfield

#ifndef RENDER_BORDER_H
#define RENDER_BORDER_H

#include "types/mino.h"

void initBorderRenderer(void);
void cleanupBorderRenderer(void);

// Add border segments to the darray
void queueBorder(enum mino playfield[PLAYFIELD_H][PLAYFIELD_W]);

// Render all border segments in one go, clear the darray
void renderBorder(void);

#endif //RENDER_BORDER_H
