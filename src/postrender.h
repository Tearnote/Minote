// Minote - postrender.h
// Handles rendering of post-processing effects

#ifndef POSTRENDER_H
#define POSTRENDER_H

void initPostRenderer(void);
void cleanupPostRenderer(void);

void resizePostRender(int width, int height);

void renderPostStart(void);
void renderPostEnd(void);

#endif //POSTRENDER_H