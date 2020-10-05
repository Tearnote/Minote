/**
 * Implementation of effects.h
 * @file
 */

#include "particles.hpp"

#include <stdbool.h>
#include <time.h>
#include "cephes/protos.h"
#include "base/varray.hpp"
#include "model.hpp"
#include "base/util.hpp"

using namespace minote;

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
	EasingFunction<float> ease; ///< Easing profile of the particle progress
} Particle;

constexpr std::size_t MaxParticles{4096};

static varray<Particle, MaxParticles> particles{};
static Rng rng{};

static Model* particle = nullptr;
static varray<color4, MaxParticles> particleTints{};
static varray<mat4x4, MaxParticles> particleTransforms{};

static bool initialized = false;

#include "meshes/particle.mesh"
void particlesInit(void)
{
	if (initialized) return;

	rng.seed((uint64_t)time(nullptr));
	particle = modelCreateFlat("particle", particleMeshSize, particleMesh);

	initialized = true;
}

void particlesCleanup(void)
{
	if (!initialized) return;

	modelDestroy(particle);
	particle = nullptr;

	initialized = false;
}

void particlesUpdate(void)
{
	ASSERT(initialized);

	size_t numParticles = particles.size;
	nsec currentTime = Window::getTime();

	for (size_t i = numParticles - 1; i < numParticles; i -= 1) {
		Particle* currentParticle = &particles[i];
		if (currentParticle->start + currentParticle->duration < currentTime)
			particles.removeSwap(i);
	}
}

void particlesDraw(void)
{
	ASSERT(initialized);

	double fresnelConst = sqrt(4.0 / Tau);

	size_t numParticles = particles.size;
	if (!numParticles) return;

	for (size_t i = 0; i < numParticles; i += 1) {
		Particle* current = &particles[i];
		ASSERT(current->spin > 0.0f);

		Tween<float> progressTween = {
			.from = 0.0f,
			.to = 1.0f,
			.start = current->start,
			.duration = current->duration,
			.type = current->ease
		};
		float progress = progressTween.apply();
		ASSERT(progress >= 0.0f && progress <= 1.0f);

		double x;
		double y;
		ASSERT(current->spin > 0.0f);
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

		color4* tint = particleTints.produce();
		ASSERT(tint);
		*tint = current->color;

		// Shimmer mitigation
		if (progress > ShimmerFade) {
			float fadeout = progress - ShimmerFade;
			fadeout *= 1.0f / (1.0f - ShimmerFade);
			fadeout = 1.0f - fadeout;
			fadeout = cubicEaseIn(fadeout);
			tint->a *= fadeout;
		}

		mat4x4* transform = particleTransforms.produce();
		ASSERT(transform);
		mat4x4 translated = {0};
		mat4x4_translate(translated, x, y, current->origin.z);
		mat4x4 rotated = {0};
		mat4x4_rotate_Z(rotated, translated, angle);
		mat4x4_scale_aniso(*transform, rotated, 1.0f - progress, 1.0f, 1.0f);
	}

	numParticles = particleTransforms.size;
	ASSERT(particleTints.size == particleTransforms.size);

	modelDraw(particle, numParticles,
		particleTints.data(),
		nullptr,
		particleTransforms.data());

	particleTints.clear();
	particleTransforms.clear();
}

void particlesGenerate(point3f position, size_t count, ParticleParams* params)
{
	ASSERT(initialized);
	ASSERT(count);
	ASSERT(params);

	for (size_t i = 0; i < count; i += 1) {
		Particle* newParticle = particles.produce();
		if (!newParticle)
			return;

		newParticle->origin = position;
		newParticle->color = params->color;

		newParticle->start = Window::getTime();
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
		newParticle->spin = quarticEaseIn(newParticle->spin);
		newParticle->spin *= params->spinMax - params->spinMin;
		newParticle->spin += params->spinMin;
	}
}
