// Minote - render/particle.c

#include "render/particle.h"

#include "glad/glad.h"

#include "AHEasing/easing.h"

#include "types/array.h"
#include "util/util.h"
#include "util/log.h"
#include "util/timer.h"
#include "global/effects.h"
#include "render/render.h"
#include "render/ease.h"
#include "render/post.h"

#define INSTANCE_LIMIT 2560 // More particles than that will be ignored
#define FADE_THRESHOLD 0.9f
#define COLOR_BOOST 3.0f

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

static GLfloat vertexData[] = {
	0.0f, -0.5f,
	1.0f, -0.5f,
	1.0f, 0.5f,

	0.0f, -0.5f,
	1.0f, 0.5f,
	0.0f, 0.5f
};

struct particle {
	float x, y;
	float progress;
	int direction; // -1 is left, 1 is right
	float radius; // positive is up, negative is down
	float spins; // in radians
	float r, g, b, a;
};

struct particleInstance {
	float x, y;
	float w, h;
	float direction; // radians
	float r, g, b, a;
};

static psarray *particleQueue = NULL;
static darray *instanceQueue = NULL;

rng randomizer = {};

void initParticleRenderer(void)
{
	srandom(&randomizer, (uint64_t)time(NULL));
	particleQueue = createPsarray(sizeof(struct particle), INSTANCE_LIMIT);
	instanceQueue = createDarray(sizeof(struct particleInstance));

	const GLchar vertSrc[] = {
#include "particle.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "particle.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize particle renderer");
	cameraAttr = glGetUniformLocation(program, "camera");
	projectionAttr = glGetUniformLocation(program, "projection");

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &instanceBuffer);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2,
	                      (GLvoid *)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 9,
	                      (GLvoid *)0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 9,
	                      (GLvoid *)(sizeof(GLfloat) * 2));
	glVertexAttribDivisor(2, 1);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 9,
	                      (GLvoid *)(sizeof(GLfloat) * 4));
	glVertexAttribDivisor(3, 1);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 9,
	                      (GLvoid *)(sizeof(GLfloat) * 5));
	glVertexAttribDivisor(4, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void cleanupParticleRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &instanceBuffer);
	instanceBuffer = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;
	destroyDarray(instanceQueue);
	instanceQueue = NULL;
	destroyPsarray(particleQueue);
	particleQueue = NULL;
}

void triggerLineClear(struct lineClearEffectData *data)
{
	for (int y = 0; y < PLAYFIELD_H; y += 1) {
		if (!data->clearedLines[y])
			continue;
		for (int i = 0; i < (data->lines + 1) / 2; i += 1) {
			for (int my = 0; my < 8; my += 1) {
				for (int x = 0; x < PLAYFIELD_W; x += 1) {
					struct particle *newParticle =
						producePsarrayItem(
							particleQueue);
					if (!newParticle)
						continue;
					newParticle->x = x - PLAYFIELD_W / 2;
					newParticle->y = PLAYFIELD_H - 1 - y;
					newParticle->y += my * 0.125f + 0.0625f;
					newParticle->progress = 0.0f;
					newParticle->direction =
						(int)random(&randomizer, 2) * 2
						- 1;
					newParticle->radius =
						frandom(&randomizer);
					newParticle->radius =
						ExponentialEaseInOut(
							newParticle->radius);
					newParticle->radius *= 2.0f;
					newParticle->radius -= 1.0f;
					newParticle->radius *= 64.0f;
					newParticle->spins =
						frandom(&randomizer);
					newParticle->spins = QuadraticEaseOut(
						newParticle->spins);
					int power = 3;
					power += data->combo * 2;
					if (power > 20 || data->lines == 4)
						power = 20;
					newParticle->spins *= (float)power;
					newParticle->spins /=
						fabsf(newParticle->radius);
					enum mino type = data->playfield[y][x];
					assert(type != MinoNone);
					newParticle->r = minoColors[type][0];
					newParticle->g = minoColors[type][1];
					newParticle->b = minoColors[type][2];
					newParticle->a = minoColors[type][3];
					double duration = frandom(&randomizer);
					if (data->lines == 4)
						duration =
							duration / 2.0f + 0.5f;
					duration *= 2.0 * SEC;
					enum easeType ease = EaseOutExponential;
					if (data->lines == 4)
						ease = EaseInOutExponential;
					addEase(&newParticle->progress, 0.0f,
					        1.0f,
					        duration, ease);
				}
			}
		}
	}

	if (data->lines == 4)
		pulseVignette();
}

void triggerThump(struct thumpEffectData *data)
{
	for (int i = 0; i < 8; i += 1) {
		struct particle
			*newParticle = producePsarrayItem(particleQueue);
		if (!newParticle)
			return;
		newParticle->x = data->x - PLAYFIELD_W / 2;
		newParticle->y = PLAYFIELD_H - 1 - data->y;
		newParticle->progress = 0.0f;
		newParticle->direction = (i % 2) * 2 - 1;
		newParticle->radius = 0.5f + 0.5f * frandom(&randomizer);
		newParticle->radius *= 8.0f;
		newParticle->spins = frandom(&randomizer);
		newParticle->spins *= 2.0f;
		newParticle->spins /= fabsf(newParticle->radius);
		newParticle->r = 0.5f;
		newParticle->g = 0.5f;
		newParticle->b = 0.5f;
		newParticle->a = 1.0f;
		double duration = 0.5 + 0.5 * frandom(&randomizer);
		duration *= 0.5 * SEC;
		enum easeType type = EaseOutExponential;
		addEase(&newParticle->progress, 0.0f, 1.0f, duration, type);
	}
}

void triggerSlide(struct slideEffectData *data)
{
	for (int i = 0; i < 8; i += 1) {
		struct particle
			*newParticle = producePsarrayItem(particleQueue);
		if (!newParticle)
			return;
		newParticle->x = data->x - PLAYFIELD_W / 2;
		newParticle->y = PLAYFIELD_H - 1 - data->y;
		newParticle->progress = 0.0f;
		newParticle->direction = data->direction;
		newParticle->radius = 0.5f + 0.5f * frandom(&randomizer);
		newParticle->radius *= 8.0f;
		newParticle->spins = frandom(&randomizer);
		newParticle->spins *= 2.0f;
		newParticle->spins /= fabsf(newParticle->radius);
		if (data->strong) {
			newParticle->r = 1.0f;
			newParticle->g = 0.0f;
			newParticle->b = 0.0f;
			newParticle->a = 1.0f;
		} else {
			newParticle->r = 0.0f;
			newParticle->g = 0.0f;
			newParticle->b = 1.0f;
			newParticle->a = 1.0f;
		}
		double duration = 0.5 + 0.5 * frandom(&randomizer);
		duration *= 0.5 * SEC;
		enum easeType type = EaseOutExponential;
		addEase(&newParticle->progress, 0.0f, 1.0f, duration, type);
	}
}

void triggerBravo(void)
{
	for (int x = 0; x < PLAYFIELD_W; x += 1) {
		for (int y = 0; y < PLAYFIELD_H_VISIBLE; y += 1) {
			struct particle
				*newParticle =
				producePsarrayItem(particleQueue);
			if (!newParticle)
				return;
			newParticle->x = x - PLAYFIELD_W / 2;
			newParticle->y = PLAYFIELD_H_VISIBLE - 1 - y;
			newParticle->y += frandom(&randomizer);
			newParticle->progress = 0.0f;
			newParticle->direction = ((x + y) % 2) * 2 - 1;
			newParticle->radius = frandom(&randomizer);
			newParticle->radius = ExponentialEaseInOut(newParticle->radius);
			newParticle->radius *= 2.0f;
			newParticle->radius -= 1.0f;
			newParticle->radius *= 8.0f;
			newParticle->spins = frandom(&randomizer);
			newParticle->spins *= 4.0f;
			newParticle->spins /= fabsf(newParticle->radius);
			newParticle->r = 1.0f;
			newParticle->g = 1.0f;
			newParticle->b = 1.0f;
			newParticle->a = 0.75f;
			double duration = 0.5 + 0.5 * frandom(&randomizer);
			duration *= 4.0 * SEC;
			enum easeType type = EaseOutExponential;
			addEase(&newParticle->progress, 0.0f, 1.0f, duration,
			        type);
		}
	}
}

void updateParticles(void)
{
	if (isPsarrayEmpty(particleQueue))
		return;
	for (int i = 0; i < particleQueue->count; i += 1) {
		struct particle *particle = getPsarrayItem(particleQueue, i);
		if (!isPsarrayItemAlive(particleQueue, i))
			continue;
		if (particle->progress == 1.0f) {
			killPsarrayItem(particleQueue, i);
		}

		struct particleInstance
			*newInstance = produceDarrayItem(instanceQueue);
		float angle = particle->progress * particle->spins;
		angle -= radf(90.0f);
		newInstance->x = cosf(angle) * particle->radius;
		if (particle->direction == -1) {
			newInstance->x *= -1.0f;
			newInstance->x += 1.0f;
		}
		newInstance->x += (float)particle->x;
		newInstance->y = sinf(angle) * particle->radius;
		newInstance->y += particle->radius;
		newInstance->y += (float)particle->y;
		newInstance->w = 1.0f - particle->progress;
		newInstance->w *= 1.1f;
		newInstance->h = 0.125f;
		newInstance->direction = angle - radf(90.0f);
		if (particle->direction == -1)
			newInstance->direction =
				radf(180.0f) - newInstance->direction;
		newInstance->r = particle->r;
		newInstance->r += tintColor[0];
		newInstance->r /= 2.0f;
		newInstance->r *= COLOR_BOOST;
		newInstance->g = particle->g;
		newInstance->g += tintColor[1];
		newInstance->g /= 2.0f;
		newInstance->g *= COLOR_BOOST;
		newInstance->b = particle->b;
		newInstance->b += tintColor[2];
		newInstance->b /= 2.0f;
		newInstance->b *= COLOR_BOOST;
		newInstance->a = particle->a;
		newInstance->a *= 0.8f;
		if (particle->progress > FADE_THRESHOLD) {
			float fadeout = particle->progress - FADE_THRESHOLD;
			fadeout *= 1.0f / (1.0f - FADE_THRESHOLD);
			fadeout = 1.0f - fadeout;
			//fadeout = LinearInterpolation(fadeout);
			newInstance->a *= fadeout;
		}
	}
}

void renderParticles(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER,
	             INSTANCE_LIMIT * sizeof(struct particleInstance), NULL,
	             GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)min(instanceQueue->count, INSTANCE_LIMIT)
	                * sizeof(struct particleInstance),
	                instanceQueue->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(program);
	glBindVertexArray(vao);
	glDisable(GL_DEPTH_TEST);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, countof(vertexData) / 2,
	                      (GLsizei)instanceQueue->count);

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glUseProgram(0);

	clearDarray(instanceQueue);
}
