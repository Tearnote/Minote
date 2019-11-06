// Minote - textrender.h
// Renders text using Multi-channel Signed Distance Fields

#ifndef TEXT_H
#define TEXT_H

#include "logic/menu.h"
#include "logic/gameplay.h"

void initTextRenderer(void);
void cleanupTextRenderer(void);

void queueMenuText(struct menu *menu);
void queueGameplayText(struct game *game);
void renderText(void);

#endif //TEXT_H