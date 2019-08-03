// Minote - minorender.h
// Renders minos to the screen

#ifndef MINORENDER_H
#define MINORENDER_H

#include "mino.h"
#include "state.h"

void initMinoRenderer(void);
void cleanupMinoRenderer(void);

// Add various parts of the gameplay to the queue of minos to render
void queueMinoPlayfield(enum mino playfield[PLAYFIELD_H][PLAYFIELD_W]);
void queueMinoPlayer(struct playerState *player);
void queueMinoPreview(struct playerState *player);

// Render everything in the queue with a single draw call, clear the queue
void renderMino(void);

#endif // MINORENDER_H
