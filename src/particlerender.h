// Minote - particlerender.h
// Spawns and renders particle effects

#ifndef PARTICLERENDER_H
#define PARTICLERENDER_H

#include <stdbool.h>

#include "effects.h"

void initParticleRenderer(void);
void cleanupParticleRenderer(void);

void triggerLineClear(struct lineClearData *data);

void updateParticles(void);
void renderParticles(void);

#endif //PARTICLERENDER_H