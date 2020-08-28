/**
 * Implementation of effects.h
 * @file
 */

#include "particles.h"

#include <stdbool.h>
#include <assert.h>
#include "darray.h"
#include "util.h"

typedef struct Particle {
	int foo;
} Particle;

static darray* particles = null;

static bool initialized = false;

void particlesInit(void)
{
	if (initialized) return;

	particles = darrayCreate(sizeof(Particle));

	initialized = true;
}

void particlesCleanup(void)
{
	if (!initialized) return;

	darrayDestroy(particles);
	particles = null;

	initialized = false;
}

void particlesUpdate(void)
{
	assert(initialized);
}

void particlesDraw(void)
{
	assert(initialized);
}

void particlesGenerate(point3f position, size_t count, ParticleParams* params)
{
	assert(initialized);
}
