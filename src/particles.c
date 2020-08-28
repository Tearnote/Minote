/**
 * Implementation of effects.h
 * @file
 */

#include "particles.h"

#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include "darray.h"
#include "model.h"
#include "util.h"

/**
 * Logical description of a particle. Does not change throughout the particle's
 * lifetime, and current position can be calculated for any point in time.
 */
typedef struct Particle {
	point3f origin; ///< Starting point
	color4 color; ///< Individual tint
	int direction; ///< -1 for left, 1 for right
	nsec start; ///< Timestamp of spawning
	nsec duration; ///< Total lifetime
	EaseType ease; ///< Easing profile of the particle progress
} Particle;

static darray* particles = null;
static Rng* rng = null;

static Model* particle = null;
static darray* particleTints = null;
static darray* particleTransforms = null;

static bool initialized = false;

void particlesInit(void)
{
	if (initialized) return;

	particles = darrayCreate(sizeof(Particle));
	rng = rngCreate((uint64_t)time(null));

	particle = modelCreateFlat(u8"particle",
#include "meshes/particle.mesh"
	);
	particleTints = darrayCreate(sizeof(color4));
	particleTransforms = darrayCreate(sizeof(mat4x4));

	initialized = true;
}

void particlesCleanup(void)
{
	if (!initialized) return;

	darrayDestroy(particleTransforms);
	particleTransforms = null;
	darrayDestroy(particleTints);
	particleTints = null;
	modelDestroy(particle);
	particle = null;

	rngDestroy(rng);
	rng = null;
	darrayDestroy(particles);
	particles = null;

	initialized = false;
}

void particlesUpdate(void)
{
	assert(initialized);

	size_t numParticles = darraySize(particles);
	nsec currentTime = getTime();

	for (size_t i = numParticles - 1; i < numParticles; i -= 1) {
		Particle* currentParticle = darrayGet(particles, i);
		if (currentParticle->start + currentParticle->duration < currentTime)
			darrayRemoveSwap(particles, i);
	}
}

void particlesDraw(void)
{
	assert(initialized);

	size_t numParticles = darraySize(particles);
	if (!numParticles) return;

	for (size_t i = 0; i < numParticles; i += 1) {
		Particle* currentParticle = darrayGet(particles, i);

		Ease ease = {
			.from = 0.0f,
			.to = 1.0f,
			.start = currentParticle->start,
			.duration = currentParticle->duration,
			.type = currentParticle->ease
		};
		float progress = easeApply(&ease);
		if (progress < 0.0f || progress > 1.0f) assert(false);

		color4* tint = darrayProduce(particleTints);
		color4Copy(*tint, currentParticle->color);

		mat4x4* transform = darrayProduce(particleTransforms);
		mat4x4 translated = {0};
		bool flip = (currentParticle->direction == -1);
		mat4x4_translate(translated,
			currentParticle->origin.x + (flip ? 1.0f : 0.0f),
			currentParticle->origin.y,
			currentParticle->origin.z);
		mat4x4 rotated = {0};
		mat4x4_rotate_Z(rotated, translated, radf(flip ? 180 : 0));
		mat4x4_scale_aniso(*transform, rotated, 1.0f - progress, 1.0f, 1.0f);
	}

	numParticles = darraySize(particleTransforms);
	assert(darraySize(particleTints) == darraySize(particleTransforms));

	modelDraw(particle, numParticles,
		darrayData(particleTints),
		null,
		darrayData(particleTransforms));

	darrayClear(particleTints);
	darrayClear(particleTransforms);
}

void particlesGenerate(point3f position, size_t count, ParticleParams* params)
{
	assert(initialized);
	assert(count);
	assert(params);
	for (size_t i = 0; i < count; i += 1) {
		Particle* newParticle = darrayProduce(particles);
		structCopy(newParticle->origin, position);
		color4Copy(newParticle->color, params->color);
		newParticle->direction = (int)rngInt(rng, 2) * 2 - 1;
		newParticle->start = getTime();
		newParticle->duration = params->durationMin + rngFloat(rng)
			* (double)(params->durationMax - params->durationMin);
		newParticle->ease = params->ease;
	}
}
