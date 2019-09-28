// Minote - particlerender.h
// Spawns and renders particle effects

#ifndef PARTICLERENDER_H
#define PARTICLERENDER_H

#include <stdbool.h>

#include "gameplay.h"

void initParticleRenderer(void);
void cleanupParticleRenderer(void);

void triggerLineClear(const bool lines[PLAYFIELD_H]);

void updateParticles(void);
void renderParticles(void);

#endif //PARTICLERENDER_H
