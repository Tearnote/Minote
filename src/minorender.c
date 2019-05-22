#include "minorender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <string.h>

#include "linmath.h"
#include "render.h"
#include "log.h"
#include "queue.h"
#include "state.h"

#define MINO_SIZE 24
#define INSTANCE_LIMIT 256

static GLuint program = 0;
static GLuint vertexAttrs = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;
static GLint projectionAttr = 0;
static GLfloat vertexData[] = {
	0, 0,
	0, MINO_SIZE,
	MINO_SIZE, 0,
	MINO_SIZE, MINO_SIZE
};

typedef struct {
	GLfloat x, y;
	GLfloat r, g, b, a;
} minoInstance;
queue* minoQueue = NULL;

void initMinoRenderer(void) {
	const GLchar vertSrc[] = {
		#include "mino.vert"
	, 0x00};
	const GLchar fragSrc[] = {
		#include "mino.frag"
	, 0x00};
	program = createProgram(vertSrc, fragSrc);
	if(program == 0)
		logError("Failed to initialize mino renderer");
	projectionAttr = glGetUniformLocation(program, "projection");
	glGenVertexArrays(1, &vertexAttrs);
	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &instanceBuffer);
	glBindVertexArray(vertexAttrs);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, INSTANCE_LIMIT * sizeof(minoInstance), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), 0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	minoQueue = createQueue(sizeof(minoInstance));
}

void cleanupMinoRenderer(void) {
	destroyQueue(minoQueue);
	minoQueue = NULL;
	destroyProgram(program);
	program = 0;
}

void queueMinoPlayfield(mino playfield[][PLAYFIELD_W]) {
	clearQueue(minoQueue);
	for(int y = 0; y < PLAYFIELD_H; y++)
	for(int x = 0; x < PLAYFIELD_W; x++) {
		mino minoType = playfield[y][x];
		if(minoType == MinoNone) continue;
		minoInstance* newInstance = produceQueueItem(minoQueue);
		newInstance->x = (GLfloat)(x * MINO_SIZE);
		newInstance->y = (GLfloat)(y * MINO_SIZE);
		memcpy(&newInstance->r, &minoColors[minoType], sizeof(vec4));
	}
}

void renderMino(void) {
	glUseProgram(program);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, &projection[0][0]);
	glBindVertexArray(vertexAttrs);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(minoQueue->count * sizeof(minoInstance)), minoQueue->buffer);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)minoQueue->count);
	glBindVertexArray(0);
}