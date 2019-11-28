// Minote - render/text.h
// Renders text using Multi-channel Signed Distance Fields

#ifndef RENDER_TEXT_H
#define RENDER_TEXT_H

#include "types/menu.h"
#include "types/game.h"

void initTextRenderer(void);
void cleanupTextRenderer(void);

void queueMenuText(struct menu *menu);
void queueGameplayText(struct game *game);
void renderText(void);

#endif //RENDER_TEXT_H
