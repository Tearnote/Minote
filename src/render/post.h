// Minote - postrender.h
// Handles rendering of post-processing effects

#ifndef POST_H
#define POST_H

void initPostRenderer(void);
void cleanupPostRenderer(void);

void resizePostRender(int width, int height);

void pulseVignette(void);

void renderPostStart(void);
void renderPostEnd(void);

#endif //POST_H