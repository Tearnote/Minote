// Minote - render/post.h
// Handles rendering of post-processing effects

#ifndef RENDER_POST_H
#define RENDER_POST_H

void initPostRenderer(void);
void cleanupPostRenderer(void);

void resizePostRender(int width, int height);

void pulseVignette(void);

void renderPostStart(void);
void renderPostEnd(void);

#endif //RENDER_POST_H
