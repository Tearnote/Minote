/**
 * Implementation of effects.h
 * @file
 */

#include "particles.h"

#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <aheasing/easing.h>
#include "darray.h"
#include "model.h"
#include "util.h"

/// Progress level after which a particle begins to fade out
#define ShimmerFade 0.9f

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
	float radius; ///< Size of the spiral path of the particle
	float spins; ///< Number of times the spiral path goes around
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
		Particle* current = darrayGet(particles, i);

		float progress = easeApply(&(Ease){
			.from = 0.0f,
			.to = 1.0f,
			.start = current->start,
			.duration = current->duration,
			.type = current->ease
		});
		assert(progress >= 0.0f && progress <= 1.0f);

		bool flip = (current->direction == -1);
		float angle = progress * current->spins - radf(90.0f);
		float x = cosf(angle) * current->radius;
		if (flip) x = x * -1.0f + 1.0f;
		x += current->origin.x;
		float y =
			current->origin.y + current->radius + sinf(angle) * current->radius;

		angle -= radf(90.0f);
		if (flip) angle = radf(180.0f) - angle;

		color4* tint = darrayProduce(particleTints);
		color4Copy(*tint, current->color);

		// Shimmer mitigation
		if (progress > ShimmerFade) {
			float fadeout = progress - ShimmerFade;
			fadeout *= 1.0f / (1.0f - ShimmerFade);
			fadeout = 1.0f - fadeout;
			fadeout = CubicEaseIn(fadeout);
			tint->a *= fadeout;
		}

		mat4x4* transform = darrayProduce(particleTransforms);
		mat4x4 translated = {0};
		mat4x4_translate(translated, x, y, current->origin.z);
		mat4x4 rotated = {0};
		mat4x4_rotate_Z(rotated, translated, angle);
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
		if (params->directionHorz != 0)
			newParticle->direction = params->directionHorz;
		else
			newParticle->direction = (int)rngInt(rng, 2) * 2 - 1;
		newParticle->start = getTime();
		newParticle->duration = params->durationMin + rngFloat(rng)
			* (double)(params->durationMax - params->durationMin);
		newParticle->ease = params->ease;
		newParticle->radius = rngFloat(rng);
		newParticle->radius = QuarticEaseOut(newParticle->radius);
		newParticle->radius /= 4.0f;
		newParticle->radius *= 3.0f;
		newParticle->radius += 0.25f;
		if (params->directionVert == -1
			|| (params->directionVert == 0 && rngInt(rng, 2)))
			newParticle->radius *= -1.0f;
		newParticle->radius *= params->radius;
		newParticle->spins = rngFloat(rng);
		newParticle->spins = QuadraticEaseOut(newParticle->spins);
		newParticle->spins *= params->power;
		newParticle->spins /= fabsf(newParticle->radius);
	}
}
