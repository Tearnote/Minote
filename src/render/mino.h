// Minote - minorender.h
// Renders minos to the screen

#ifndef MINORENDER_H
#define MINORENDER_H

#include "types/mino.h"
#include "global/state.h"

void initMinoRenderer(void);
void cleanupMinoRenderer(void);

void triggerLockFlash(int coords[MINOS_PER_PIECE * 2]);

// Add various parts of the gameplay to the darray of minos to render
void queueMinoPlayfield(enum mino playfield[PLAYFIELD_H][PLAYFIELD_W]);
void queueMinoPlayer(struct player *player);
void queueMinoGhost(struct player *player);
void queueMinoPreview(struct player *player);

// Queues a single invisible mino for the purpose of pipeline sync
void queueMinoSync(void);

// Render everything in the darray with a single draw call, clear the darray
void renderMino(void);

#endif //MINO_H