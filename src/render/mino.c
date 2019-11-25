// Minote - render/mino.c

#include "render/mino.h"

#include <math.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath/linmath.h"

#include "types/array.h"
#include "types/mino.h"
#include "types/game.h"
#include "util/log.h"
#include "util/util.h"
#include "logic/logic.h"
#include "render/render.h"
#include "render/ease.h"

#define INSTANCE_LIMIT 256 // More minos than that will be ignored

#define STACK_DIM 0.4f
#define LOCKDIM_STRENGTH 0.6f
#define FLASH_STRENGTH 1.2f
#define GHOST_OPACITY 0.2f

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
static GLint ambientColorAttr = -1;
static GLint diffuseStrengthAttr = -1;
static GLint specularStrengthAttr = -1;
static GLint shininessAttr = -1;
static GLint highlightMaxAttr = -1;

static mat4x4 normalCamera = {};

static GLfloat vertexData[] = {
#include "mino.vtx"
};

// Rendering-ready representation of a mino
struct minoInstance {
	GLfloat x, y;
	GLfloat r, g, b, a;
	GLfloat highlight;
};
static darray *minoQueue = NULL;

// Strength of the highlight effect on each block
float highlights[PLAYFIELD_H][PLAYFIELD_W] = {};

void initMinoRenderer(void)
{
	minoQueue = createDarray(sizeof(struct minoInstance));

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
	ambientColorAttr = glGetUniformLocation(program, "ambientColor");
	diffuseStrengthAttr = glGetUniformLocation(program, "diffuseStrength");
	specularStrengthAttr =
		glGetUniformLocation(program, "specularStrength");
	shininessAttr = glGetUniformLocation(program, "shininess");
	highlightMaxAttr = glGetUniformLocation(program, "highlightMax");

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
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
	                      (GLvoid *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
	                      (GLvoid *)(sizeof(GLfloat) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7,
	                      (GLvoid *)0);
	glVertexAttribDivisor(2, 1);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7,
	                      (GLvoid *)(sizeof(GLfloat) * 2));
	glVertexAttribDivisor(3, 1);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7,
	                      (GLvoid *)(sizeof(GLfloat) * 6));
	glVertexAttribDivisor(4, 1);
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
	destroyDarray(minoQueue);
	minoQueue = NULL;
}

void triggerLockFlash(int coords[MINOS_PER_PIECE * 2])
{
	int flashDuration =
		//player->laws.clearOffset * 2 * (SEC / logicFrequency);
		4 * 2 * (SEC / logicFrequency); //TODO replace 4 with clearOffset
	for (int i = 0; i < MINOS_PER_PIECE; i += 1) {
		addEase(&highlights[coords[i * 2 + 1]][coords[i * 2]],
		        1.0f, 0.0f, flashDuration, EaseLinear);
	}
}

void queueMinoPlayfield(enum mino field[PLAYFIELD_H][PLAYFIELD_W])
{
	for (int y = PLAYFIELD_H_HIDDEN; y < PLAYFIELD_H; y += 1) {
		for (int x = 0; x < PLAYFIELD_W; x += 1) {
			enum mino minoType = field[y][x];
			if (minoType == MinoNone)
				continue;
			struct minoInstance
				*newInstance = produceDarrayItem(minoQueue);
			newInstance->x = (GLfloat)(x - PLAYFIELD_W / 2.0);
			newInstance->y = (GLfloat)(PLAYFIELD_H - 1 - y);
			newInstance->r = minoColors[minoType][0] * STACK_DIM;
			newInstance->g = minoColors[minoType][1] * STACK_DIM;
			newInstance->b = minoColors[minoType][2] * STACK_DIM;
			newInstance->a = minoColors[minoType][3];
			newInstance->highlight = highlights[y][x];
		}
	}
}

void queueMinoPlayer(struct player *player)
{
	if (player->state != PlayerActive)
		return;

	for (int i = 0; i < MINOS_PER_PIECE; i += 1) {
		struct coord minoCoord = rs[player->type][player->rotation][i];
		struct minoInstance *newInstance = produceDarrayItem(minoQueue);
		float lockDim =
			(float)player->lockDelay / player->laws.lockDelay;
		lockDim *= LOCKDIM_STRENGTH;
		lockDim = 1.0f - lockDim;
		newInstance->x =
			(GLfloat)(minoCoord.x + player->x - PLAYFIELD_W / 2.0);
		newInstance->y =
			(GLfloat)(PLAYFIELD_H - 1 - minoCoord.y - player->y);
		newInstance->r = minoColors[player->type][0] * lockDim;
		newInstance->g = minoColors[player->type][1] * lockDim;
		newInstance->b = minoColors[player->type][2] * lockDim;
		newInstance->a = minoColors[player->type][3];
		newInstance->highlight = 0.0f;
	}
}

void queueMinoGhost(struct player *player)
{
	if (!player->laws.ghost)
		return;
	if (player->state != PlayerActive && player->state != PlayerSpawned)
		return;

	for (int i = 0; i < MINOS_PER_PIECE; i += 1) {
		struct coord minoCoord = rs[player->type][player->rotation][i];
		struct minoInstance *newInstance = produceDarrayItem(minoQueue);
		newInstance->x =
			(GLfloat)(minoCoord.x + player->x - PLAYFIELD_W / 2.0);
		newInstance->y =
			(GLfloat)(PLAYFIELD_H - 1 - minoCoord.y
			          - player->yGhost);
		newInstance->r = minoColors[player->type][0];
		newInstance->g = minoColors[player->type][1];
		newInstance->b = minoColors[player->type][2];
		newInstance->a = minoColors[player->type][3] * GHOST_OPACITY;
		newInstance->highlight = 0.0f;
	}
}

void queueMinoPreview(struct player *player)
{
	if (player->preview == MinoNone)
		return;
	for (int i = 0; i < MINOS_PER_PIECE; i += 1) {
		struct coord minoCoord = rs[player->preview][0][i];
		struct minoInstance *newInstance = produceDarrayItem(minoQueue);
		if (player->preview == PieceI)
			minoCoord.y += 1;
		newInstance->x = (GLfloat)(minoCoord.x - PIECE_BOX / 2.0);
		newInstance->y = (GLfloat)(PLAYFIELD_H + 3 - minoCoord.y);
		newInstance->r = minoColors[player->preview][0];
		newInstance->g = minoColors[player->preview][1];
		newInstance->b = minoColors[player->preview][2];
		newInstance->a = minoColors[player->preview][3];
		newInstance->highlight = 0.0f;
	}
}

void queueMinoSync(void)
{
	produceDarrayItem(minoQueue);
}

void renderMino(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER,
	             INSTANCE_LIMIT * sizeof(struct minoInstance),
	             NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)min(minoQueue->count, INSTANCE_LIMIT)
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
	glUniform3f(lightColorAttr, 1.0f, 1.0f, 1.0f);
	glUniform1f(ambientStrengthAttr, 0.2f);
	glUniform3fv(ambientColorAttr, 1, tintColor);
	glUniform1f(diffuseStrengthAttr, 0.9f);
	glUniform1f(specularStrengthAttr, 0.4f);
	glUniform1f(shininessAttr, 8.0f);
	glUniform1f(highlightMaxAttr, FLASH_STRENGTH);
	glDrawArraysInstanced(GL_TRIANGLES, 0, countof(vertexData) / 6,
	                      (GLsizei)minoQueue->count);

	glBindVertexArray(0);
	glUseProgram(0);

	clearDarray(minoQueue);
}
