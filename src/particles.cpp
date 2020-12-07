/**
 * Implementation of effects.h
 * @file
 */

#include "particles.hpp"

#include <time.h>
#include "cephes/protos.h"
#include "base/array.hpp"
#include "sys/glfw.hpp"
#include "engine/model.hpp"
#include "base/util.hpp"
#include "base/rng.hpp"

using namespace minote;

/// Progress level after which a particle begins to fade out
#define ShimmerFade 0.9f

/**
 * Logical description of a particle. Does not change throughout the particle's
 * lifetime, and current position can be calculated for any point in time.
 */
typedef struct Particle {
	vec3 origin; ///< Starting point
	color4 color; ///< Individual tint
	int vert; ///< -1 for down, 1 for up
	int horz; ///< -1 for left, 1 for right
	nsec start; ///< Timestamp of spawning
	nsec duration; ///< Total lifetime
	float distance; ///< Total distance travelled from origin
	float spin; ///< Rate at which the particle turns
	EasingFunction<float> ease; ///< Easing profile of the particle progress
} Particle;

constexpr size_t MaxParticles{4096};

static svector<Particle, MaxParticles> particles{};
static Rng rng{};

static svector<ModelFlat::Instance, MaxParticles> particleInstances{};

static bool initialized = false;

void particlesInit(void)
{
	if (initialized) return;

	rng.seed((uint64_t)time(nullptr));

	initialized = true;
}

void particlesUpdate(void)
{
	DASSERT(initialized);

	size_t numParticles = particles.size();
	nsec currentTime = Glfw::getTime();

	for (size_t i = numParticles - 1; i < numParticles; i -= 1) {
		Particle* currentParticle = &particles[i];
		if (currentParticle->start + currentParticle->duration < currentTime) {
			particles[i] = particles.back();
			particles.pop_back();
		}
	}
}

void particlesDraw(Engine& engine)
{
	DASSERT(initialized);

	double fresnelConst = sqrt(4.0 / Tau);

	size_t const numParticles = particles.size();
	if (!numParticles) return;

	for (size_t i = 0; i < numParticles; i += 1) {
		Particle* current = &particles[i];
		DASSERT(current->spin > 0.0f);

		Tween progressTween = {
			.from = 0.0f,
			.to = 1.0f,
			.start = current->start,
			.duration = current->duration,
			.type = current->ease
		};
		float progress = progressTween.apply();
		DASSERT(progress >= 0.0f && progress <= 1.0f);

		double x;
		double y;
		DASSERT(current->spin > 0.0f);
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
			angle = radians(180.0f) - angle;
		if (current->vert == -1)
			angle *= -1.0f;

		auto& instance = particleInstances.emplace_back();
		instance.tint = current->color;

		// Shimmer mitigation
		if (progress > ShimmerFade) {
			float fadeout = progress - ShimmerFade;
			fadeout *= 1.0f / (1.0f - ShimmerFade);
			fadeout = 1.0f - fadeout;
			fadeout = cubicEaseIn(fadeout);
			instance.tint.a *= fadeout;
		}

		const mat4 translated = make_translate({(float)x, (float)y, current->origin.z});
		const mat4 rotated = rotate(translated, angle, {0.0f, 0.0f, 1.0f});
		instance.transform = scale(rotated, {1.0f - progress, 1.0f, 1.0f});
	}

	engine.models.particle.draw(*engine.frame.fb, engine.scene, {
		.blending = true
	}, particleInstances);
	particleInstances.clear();
}

void particlesGenerate(vec3 position, size_t count, ParticleParams* params)
{
	DASSERT(initialized);
	DASSERT(count);
	DASSERT(params);

	for (size_t i = 0; i < count; i += 1) {
		auto& newParticle = particles.emplace_back();

		newParticle.origin = position;
		newParticle.color = params->color;

		newParticle.start = Glfw::getTime();
		newParticle.duration = round(params->durationMin +
			(params->durationMax - params->durationMin) * rng.randFloat());
		newParticle.ease = params->ease;

		if (params->directionHorz != 0)
			newParticle.horz = params->directionHorz;
		else
			newParticle.horz = (int)rng.randInt(2) * 2 - 1;
		if (params->directionVert != 0)
			newParticle.vert = params->directionVert;
		else
			newParticle.vert = (int)rng.randInt(2) * 2 - 1;

		newParticle.distance = rng.randFloat();
		newParticle.distance *= params->distanceMax - params->distanceMin;
		newParticle.distance += params->distanceMin;

		newParticle.spin = rng.randFloat();
		newParticle.spin = quarticEaseIn(newParticle.spin);
		newParticle.spin *= params->spinMax - params->spinMin;
		newParticle.spin += params->spinMin;
	}
}
