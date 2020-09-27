/**
 * Implementation of effects.h
 * @file
 */

#include "particles.hpp"

#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include "aheasing/easing.h"
#include "cephes/protos.h"
#include "darray.hpp"
#include "model.hpp"
#include "util.hpp"

/// Progress level after which a particle begins to fade out
#define ShimmerFade 0.9f

/**
 * Logical description of a particle. Does not change throughout the particle's
 * lifetime, and current position can be calculated for any point in time.
 */
typedef struct Particle {
	point3f origin; ///< Starting point
	color4 color; ///< Individual tint
	int vert; ///< -1 for down, 1 for up
	int horz; ///< -1 for left, 1 for right
	nsec start; ///< Timestamp of spawning
	nsec duration; ///< Total lifetime
	float distance; ///< Total distance travelled from origin
	float spin; ///< Rate at which the particle turns
	EaseType ease; ///< Easing profile of the particle progress
} Particle;

static darray* particles = nullptr;
static Rng rng{};

static Model* particle = nullptr;
static darray* particleTints = nullptr;
static darray* particleTransforms = nullptr;

static bool initialized = false;

#include "meshes/particle.mesh"
void particlesInit(void)
{
	if (initialized) return;

	particles = darrayCreate(sizeof(Particle));
	rng.seed((uint64_t)time(nullptr));

	particle = modelCreateFlat("particle", particleMeshSize, particleMesh);
	particleTints = darrayCreate(sizeof(color4));
	particleTransforms = darrayCreate(sizeof(mat4x4));

	initialized = true;
}

void particlesCleanup(void)
{
	if (!initialized) return;

	darrayDestroy(particleTransforms);
	particleTransforms = nullptr;
	darrayDestroy(particleTints);
	particleTints = nullptr;
	modelDestroy(particle);
	particle = nullptr;

	darrayDestroy(particles);
	particles = nullptr;

	initialized = false;
}

void particlesUpdate(void)
{
	assert(initialized);

	size_t numParticles = particles->count;
	nsec currentTime = getTime();

	for (size_t i = numParticles - 1; i < numParticles; i -= 1) {
		Particle* currentParticle = static_cast<Particle*>(darrayGet(particles,
			i));
		if (currentParticle->start + currentParticle->duration < currentTime)
			darrayRemoveSwap(particles, i);
	}
}

void particlesDraw(void)
{
	assert(initialized);

	double fresnelConst = sqrt(4.0 / Tau);

	size_t numParticles = particles->count;
	if (!numParticles) return;

	for (size_t i = 0; i < numParticles; i += 1) {
		Particle* current = static_cast<Particle*>(darrayGet(particles, i));
		assert(current->spin > 0.0f);

		Tween progressTween = {
			.from = 0.0f,
			.to = 1.0f,
			.start = current->start,
			.duration = current->duration,
			.type = current->ease
		};
		float progress = tweenApply(&progressTween);
		assert(progress >= 0.0f && progress <= 1.0f);

		double x;
		double y;
		assert(current->spin > 0.0f);
		float distance = progress * current->distance * current->spin;
		fresnl(distance * fresnelConst, &y, &x);
		x = x / fresnelConst / current->spin;
		y = y / fresnelConst / current->spin;
		if (current->horz == -1)
			x *= -1.0;
		else
			x += 1.0;
		if (current->vert == -1)
			y *= -1.0;
		x += current->origin.x;
		y += current->origin.y;

		float angle = distance * distance;
		if (current->horz == -1)
			angle = rad(180.0f) - angle;
		if (current->vert == -1)
			angle *= -1.0f;

		color4* tint = static_cast<color4*>(darrayProduce(particleTints));
		*tint = current->color;

		// Shimmer mitigation
		if (progress > ShimmerFade) {
			float fadeout = progress - ShimmerFade;
			fadeout *= 1.0f / (1.0f - ShimmerFade);
			fadeout = 1.0f - fadeout;
			fadeout = CubicEaseIn(fadeout);
			tint->a *= fadeout;
		}

		mat4x4* transform = static_cast<mat4x4*>(darrayProduce(
			particleTransforms));
		mat4x4 translated = {0};
		mat4x4_translate(translated, x, y, current->origin.z);
		mat4x4 rotated = {0};
		mat4x4_rotate_Z(rotated, translated, angle);
		mat4x4_scale_aniso(*transform, rotated, 1.0f - progress, 1.0f, 1.0f);
	}

	numParticles = particleTransforms->count;
	assert(particleTints->count == particleTransforms->count);

	modelDraw(particle, numParticles,
		(color4*)particleTints->data,
		nullptr,
		(mat4x4*)particleTransforms->data);

	darrayClear(particleTints);
	darrayClear(particleTransforms);
}

void particlesGenerate(point3f position, size_t count, ParticleParams* params)
{
	assert(initialized);
	assert(count);
	assert(params);

	for (size_t i = 0; i < count; i += 1) {
		Particle* newParticle = static_cast<Particle*>(darrayProduce(
			particles));
		newParticle->origin = position;
		newParticle->color = params->color;

		newParticle->start = getTime();
		newParticle->duration = params->durationMin + rng.randFloat()
			* (double)(params->durationMax - params->durationMin);
		newParticle->ease = params->ease;

		if (params->directionHorz != 0)
			newParticle->horz = params->directionHorz;
		else
			newParticle->horz = (int)rng.randInt(2) * 2 - 1;
		if (params->directionVert != 0)
			newParticle->vert = params->directionVert;
		else
			newParticle->vert = (int)rng.randInt(2) * 2 - 1;

		newParticle->distance = rng.randFloat();
		newParticle->distance *= params->distanceMax - params->distanceMin;
		newParticle->distance += params->distanceMin;

		newParticle->spin = rng.randFloat();
		newParticle->spin = QuarticEaseIn(newParticle->spin);
		newParticle->spin *= params->spinMax - params->spinMin;
		newParticle->spin += params->spinMin;
	}
}
