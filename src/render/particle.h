// Minote - particlerender.h
// Spawns and renders particle effects

#ifndef PARTICLE_H
#define PARTICLE_H

#include <stdbool.h>

#include "global/effects.h"

void initParticleRenderer(void);
void cleanupParticleRenderer(void);

void triggerLineClear(struct lineClearEffectData *data);
void triggerThump(struct thumpEffectData *data);
void triggerSlide(struct slideEffectData *data);
void triggerBravo(void);

void updateParticles(void);
void renderParticles(void);

#endif //PARTICLE_H