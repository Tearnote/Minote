// Minote - particlerender.c

#include "particlerender.h"

#include <stdbool.h>
#include <string.h>

#include "glad/glad.h"

#include "gameplay.h"
#include "log.h"
#include "array.h"
#include "render.h"
#include "ease.h"
#include "timer.h"

#define INSTANCE_LIMIT 2048 // More particles than that will be ignored

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

static GLfloat vertexData[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,

	0.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f
};

struct particle {
	float x, y;
	float progress;
};

struct particleInstance {
	float x, y;
	float w, h;
	float direction; // radians
	float r, g, b, a;
};

static psarray *particleQueue = NULL;
static darray *instanceQueue = NULL;

void initParticleRenderer(void)
{
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

void triggerLineClear(const bool lines[PLAYFIELD_H])
{
	for (int i = 0; i < PLAYFIELD_H; i++) {
		if (!lines[i])
			continue;

		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < PLAYFIELD_W; x++) {
				struct particle *newParticle = producePsarrayItem(particleQueue);
				if (!newParticle)
					continue;
				newParticle->x = x - PLAYFIELD_W / 2;
				newParticle->y = PLAYFIELD_H - 1 - i;
				newParticle->y += y * 0.125f;
				newParticle->progress = 0.0f;
				addEase(&newParticle->progress, 0.0f, 1.0f,
				        1 * SEC, EaseOutQuadratic);
			}
		}
	}
}

void updateParticles(void)
{
	if (isPsarrayEmpty(particleQueue))
		return;
	for (int i = 0; i < particleQueue->count; i++) {
		struct particle *particle = getPsarrayItem(particleQueue, i);
		if (!isPsarrayItemAlive(particleQueue, i))
			continue;
		if (particle->progress == 1.0f) {
			killPsarrayItem(particleQueue, i);
		}

		struct particleInstance
			*newInstance = produceDarrayItem(instanceQueue);
		newInstance->x =
			(float)particle->x + 1.0f + particle->progress * 4;
		newInstance->y = (float)particle->y;
		newInstance->w = 1.0f;
		newInstance->h = 0.125f;
		newInstance->direction = 0.0f;
		newInstance->r = 1.0f;
		newInstance->g = 1.0f;
		newInstance->b = 1.0f;
		newInstance->a = 0.8f;
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