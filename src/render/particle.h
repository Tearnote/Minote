// Minote - render/particle.h
// Spawns and renders particle effects

#ifndef RENDER_PARTICLE_H
#define RENDER_PARTICLE_H

#include "global/effects.h"

void initParticleRenderer(void);
void cleanupParticleRenderer(void);

void triggerLineClear(struct lineClearEffectData *data);
void triggerThump(struct thumpEffectData *data);
void triggerSlide(struct slideEffectData *data);
void triggerBravo(void);

void updateParticles(void);
void renderParticles(void);

#endif //RENDER_PARTICLE_H
