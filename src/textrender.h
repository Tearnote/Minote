// Minote - textrender.h
// Renders text using Multi-channel Signed Distance Fields

#ifndef TEXTRENDER_H
#define TEXTRENDER_H

void initTextRenderer(void);
void cleanupTextRenderer(void);

void queuePlayfieldText(void);
void renderText(void);

#endif //TEXTRENDER_H