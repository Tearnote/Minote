// Minote - textrender.h
// Renders text using Multi-channel Signed Distance Fields

#ifndef TEXTRENDER_H
#define TEXTRENDER_H

#include "gameplay.h"
#include "replay.h"

void initTextRenderer(void);
void cleanupTextRenderer(void);

void queueGameplayText(struct game *game);
void queueReplayText(struct replay *replay);
void renderText(void);

#endif //TEXTRENDER_H