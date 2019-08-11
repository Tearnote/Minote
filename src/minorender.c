// Minote - minorender.c

#include "minorender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <string.h>

#include "linmath/linmath.h"
#include "render.h"
#include "log.h"
#include "queue.h"
#include "state.h"
#include "window.h"
#include "mino.h"
#include "util.h"

#define MINO_SIZE 12 // Size of the mino in pixels
#define INSTANCE_LIMIT 256 // More minos than that will be ignored

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint elementBuffer = 0;
static GLuint instanceBuffer = 0;
static GLint modelAttr = -1;
static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

static mat4x4 model = {};

static GLfloat vertexData[] = {
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, 0.75f, -1.0f,
	-1.0f, 0.75f, -1.0f,
	-1.0f, 0.75f, 1.0f,
	1.0f, 0.75f, 1.0f,
	0.75f, 1.0f, 0.75f,
	0.75f, 1.0f, -0.75f,
	-0.75f, 1.0f, -0.75f,
	-0.75f, 1.0f, 0.75f
};
static GLuint elementData[] = {
	7, 0, 1,
	6, 2, 3,
	6, 1, 7,
	0, 5, 2,
	8, 10, 9,
	4, 10, 5,
	7, 9, 4,
	7, 11, 6,
	11, 5, 6,
	7, 4, 0,
	6, 5, 2,
	6, 3, 1,
	0, 4, 5,
	8, 11, 10,
	4, 9, 10,
	7, 8, 9,
	7, 8, 11,
	11, 10, 5
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
	modelAttr = glGetUniformLocation(program, "model");
	cameraAttr = glGetUniformLocation(program, "camera");
	projectionAttr = glGetUniformLocation(program, "projection");

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elementData), elementData,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBindVertexArray(0);

	mat4x4_identity(model);
	mat4x4_scale_aniso(model, model, 0.5f, 0.5f, 0.5f);
	mat4x4_rotate_X(model, model, radf(90.0f));
}

void cleanupMinoRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &elementBuffer);
	elementBuffer = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;
	destroyQueue(minoQueue);
	minoQueue = NULL;
}

/*
void queueMinoPlayfield(enum mino field[PLAYFIELD_H][PLAYFIELD_W])
{
	// Center the playfield
	int xOffset = renderWidth / 2 - PLAYFIELD_W / 2 * MINO_SIZE;
	int yOffset = renderHeight / 2 - PLAYFIELD_H_VISIBLE / 2 * MINO_SIZE;

	for (int y = PLAYFIELD_H_HIDDEN; y < PLAYFIELD_H; y++) {
		for (int x = 0; x < PLAYFIELD_W; x++) {
			enum mino minoType = field[y][x];
			// Commenting this gives a temporary black background
			//if(minoType == MinoNone) continue;
			struct minoInstance
				*newInstance = produceQueueItem(minoQueue);
			newInstance->x = (GLfloat)(x * MINO_SIZE + xOffset);
			newInstance->y = (GLfloat)(y * MINO_SIZE + yOffset);
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

	int xOffset =
		renderWidth / 2 - (PLAYFIELD_W / 2 - player->x) * MINO_SIZE;
	int yOffset =
		renderHeight / 2
		- (PLAYFIELD_H_VISIBLE / 2 - player->y) * MINO_SIZE;

	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		struct coord minoCoord = rs[player->type][player->rotation][i];
		struct minoInstance *newInstance = produceQueueItem(minoQueue);
		newInstance->x = (GLfloat)(minoCoord.x * MINO_SIZE + xOffset);
		newInstance->y = (GLfloat)(minoCoord.y * MINO_SIZE + yOffset);
		newInstance->r = minoColors[player->type][0];
		newInstance->g = minoColors[player->type][1];
		newInstance->b = minoColors[player->type][2];
		newInstance->a = minoColors[player->type][3];
	}
}

void queueMinoPreview(struct player *player)
{
	int xOffset =
		renderWidth / 2 - PIECE_BOX / 2 * MINO_SIZE;
	int yOffset =
		renderHeight / 2
		- (PLAYFIELD_H_VISIBLE / 2 + PIECE_BOX) * MINO_SIZE;

	for (int i = 0; i < MINOS_PER_PIECE; i++) {
		struct coord minoCoord = rs[player->preview][0][i];
		struct minoInstance *newInstance = produceQueueItem(minoQueue);
		newInstance->x = (GLfloat)(minoCoord.x * MINO_SIZE + xOffset);
		newInstance->y = (GLfloat)(minoCoord.y * MINO_SIZE + yOffset);
		newInstance->r = minoColors[player->preview][0];
		newInstance->g = minoColors[player->preview][1];
		newInstance->b = minoColors[player->preview][2];
		newInstance->a = minoColors[player->preview][3];
	}
}
*/
void renderMino(void)
{
	glUseProgram(program);
	glBindVertexArray(vao);
	glUniformMatrix4fv(modelAttr, 1, GL_FALSE, model[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glDrawElements(GL_TRIANGLES, COUNT_OF(elementData), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
