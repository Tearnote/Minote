// Minote - minorender.c

#include "minorender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <string.h>
#include <math.h>

#include "linmath/linmath.h"
#include "render.h"
#include "log.h"
#include "queue.h"
#include "state.h"
#include "window.h"
#include "mino.h"
#include "util.h"

#define INSTANCE_LIMIT 256 // More minos than that will be ignored

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;

static GLint cameraAttr = -1;
static GLint normalCameraAttr = -1;
static GLint projectionAttr = -1;
static GLint lightPositionAttr = -1;
static GLint lightColorAttr = -1;
static GLint ambientStrengthAttr = -1;
static GLint diffuseStrengthAttr = -1;
static GLint specularStrengthAttr = -1;
static GLint shininessAttr = -1;

static mat4x4 normalCamera = {};

static GLfloat vertexData[] = {
#include "mino.vtx"
};

// Rendering-ready representation of a mino
struct minoInstance {
	GLfloat x, y;
	GLfloat r, g, b, a;
};
queue *minoQueue = NULL;

void initMinoRenderer(void)
{
	minoQueue = createQueue(sizeof(struct minoInstance));

	const GLchar vertSrc[] = {
#include "mino.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "mino.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize mino renderer");
	cameraAttr = glGetUniformLocation(program, "camera");
	normalCameraAttr = glGetUniformLocation(program, "normalCamera");
	projectionAttr = glGetUniformLocation(program, "projection");
	lightPositionAttr = glGetUniformLocation(program, "lightPosition");
	lightColorAttr = glGetUniformLocation(program, "lightColor");
	ambientStrengthAttr = glGetUniformLocation(program, "ambientStrength");
	diffuseStrengthAttr = glGetUniformLocation(program, "diffuseStrength");
	specularStrengthAttr = glGetUniformLocation(program, "specularStrength");
	shininessAttr = glGetUniformLocation(program, "shininess");

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &instanceBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER,
	             INSTANCE_LIMIT * sizeof(struct minoInstance),
	             NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
	                      (GLvoid *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
	                      (GLvoid *)(sizeof(GLfloat) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
	                      (GLvoid *)0);
	glVertexAttribDivisor(2, 1);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
	                      (GLvoid *)(sizeof(GLfloat) * 2));
	glVertexAttribDivisor(3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void cleanupMinoRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &instanceBuffer);
	instanceBuffer = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;
	destroyQueue(minoQueue);
	minoQueue = NULL;
}

void queueMinoPlayfield(enum mino field[PLAYFIELD_H][PLAYFIELD_W])
{
	for (int y = PLAYFIELD_H_HIDDEN; y < PLAYFIELD_H; y++) {
		for (int x = 0; x < PLAYFIELD_W; x++) {
			enum mino minoType = field[y][x];
			if (minoType == MinoNone)
				continue;
			struct minoInstance
				*newInstance = produceQueueItem(minoQueue);
			newInstance->x = (GLfloat)(x - PLAYFIELD_W / 2);
			newInstance->y = (GLfloat)(PLAYFIELD_H - 1 - y);
			newInstance->r = minoColors[minoType][0] / 5;
			newInstance->g = minoColors[minoType][1] / 5;
			newInstance->b = minoColors[minoType][2] / 5;
			newInstance->a = minoColors[minoType][3];
		}
	}
}

void queueMinoPlayer(struct player *player)
{
	if (player->state != PlayerActive)
		return;

	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		struct coord minoCoord = rs[player->type][player->rotation][i];
		struct minoInstance *newInstance = produceQueueItem(minoQueue);
		newInstance->x =
			(GLfloat)(minoCoord.x + player->x - PLAYFIELD_W / 2);
		newInstance->y =
			(GLfloat)(PLAYFIELD_H - 1 - minoCoord.y - player->y);
		newInstance->r = minoColors[player->type][0];
		newInstance->g = minoColors[player->type][1];
		newInstance->b = minoColors[player->type][2];
		newInstance->a = minoColors[player->type][3];
	}
}

void queueMinoPreview(struct player *player)
{
	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		struct coord minoCoord = rs[player->preview][0][i];
		struct minoInstance *newInstance = produceQueueItem(minoQueue);
		if (player->preview == PieceI)
			minoCoord.y += 1;
		newInstance->x = (GLfloat)(minoCoord.x - PIECE_BOX / 2);
		newInstance->y = (GLfloat)(PLAYFIELD_H + 3 - minoCoord.y);
		newInstance->r = minoColors[player->preview][0];
		newInstance->g = minoColors[player->preview][1];
		newInstance->b = minoColors[player->preview][2];
		newInstance->a = minoColors[player->preview][3];
	}
}

void renderMino(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)MIN(minoQueue->count, INSTANCE_LIMIT)
	                * sizeof(struct minoInstance), minoQueue->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	mat4x4 temp = {};
	mat4x4_invert(temp, camera);
	mat4x4_transpose(normalCamera, temp);

	glUseProgram(program);
	glBindVertexArray(vao);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(normalCameraAttr, 1, GL_FALSE, normalCamera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glUniform3fv(lightPositionAttr, 1, lightPosition);
	glUniform3f(lightColorAttr,
	            powf(1.0f, 2.2f),
	            powf(1.0f, 2.2f),
	            powf(1.0f, 2.2f));
	glUniform1f(ambientStrengthAttr, powf(0.25f, 2.2f));
	glUniform1f(diffuseStrengthAttr, powf(1.0f, 2.2f));
	glUniform1f(specularStrengthAttr, powf(0.5f, 2.2f));
	glUniform1f(shininessAttr, 32.0f);
	glDrawArraysInstanced(GL_TRIANGLES, 0, COUNT_OF(vertexData) / 6,
	                      (GLsizei)minoQueue->count);

	glBindVertexArray(0);
	glUseProgram(0);

	clearQueue(minoQueue);
}
