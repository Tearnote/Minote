// Minote - minorender.c

#include "border.h"

#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "types/array.h"
#include "logic/gameplay.h"
#include "render.h"
#include "util/log.h"
#include "util/util.h"

#define INSTANCE_LIMIT 512 // More segments than that will be ignored

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;
static GLint colorAttr = -1;

static GLfloat vertexData[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,

	0.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f
};

// Per-segment information
struct segmentInstance {
	GLfloat x1, y1, x2, y2;
};
static darray *segmentQueue = NULL;

void initBorderRenderer(void)
{
	segmentQueue = createDarray(sizeof(struct segmentInstance));

	const GLchar vertSrc[] = {
#include "border.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "border.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize mino renderer");
	cameraAttr = glGetUniformLocation(program, "camera");
	projectionAttr = glGetUniformLocation(program, "projection");
	colorAttr = glGetUniformLocation(program, "color");

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
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2,
	                      (GLvoid *)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4,
	                      (GLvoid *)0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4,
	                      (GLvoid *)(sizeof(GLfloat) * 2));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void cleanupBorderRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &instanceBuffer);
	instanceBuffer = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;
	destroyDarray(segmentQueue);
	segmentQueue = NULL;
}

static void queueBorderSegment(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	struct segmentInstance *newInstance = produceDarrayItem(segmentQueue);
	newInstance->x1 = x1;
	newInstance->y1 = y1;
	newInstance->x2 = x2;
	newInstance->y2 = y2;
}

void queueBorder(enum mino field[PLAYFIELD_H][PLAYFIELD_W])
{
	for (int y = PLAYFIELD_H_HIDDEN; y < PLAYFIELD_H; y++) {
		for (int x = 0; x < PLAYFIELD_W; x++) {
			enum mino minoType = field[y][x];
			if (minoType == MinoNone)
				continue;

			// Coords transformed to world space
			int tx = x - PLAYFIELD_W / 2;
			int ty = PLAYFIELD_H - 1 - y;

			// Left
			if (getPlayfieldGrid(field, x - 1, y) == MinoNone)
				queueBorderSegment((GLfloat)tx,
				                   (GLfloat)ty + 0.125f,
				                   (GLfloat)tx + 0.125f,
				                   (GLfloat)ty + 0.875f);

			// Right
			if (getPlayfieldGrid(field, x + 1, y) == MinoNone)
				queueBorderSegment((GLfloat)tx + 0.875f,
				                   (GLfloat)ty + 0.125f,
				                   (GLfloat)tx + 1.0f,
				                   (GLfloat)ty + 0.875f);

			// Top
			if (getPlayfieldGrid(field, x, y - 1) == MinoNone)
				queueBorderSegment((GLfloat)tx + 0.125f,
				                   (GLfloat)ty + 0.875f,
				                   (GLfloat)tx + 0.875f,
				                   (GLfloat)ty + 1.0f);

			// Bottom
			if (getPlayfieldGrid(field, x, y + 1) == MinoNone)
				queueBorderSegment((GLfloat)tx + 0.125f,
				                   (GLfloat)ty,
				                   (GLfloat)tx + 0.875f,
				                   (GLfloat)ty + 0.125f);

			// Top left
			if (getPlayfieldGrid(field, x - 1, y) == MinoNone ||
			    getPlayfieldGrid(field, x - 1, y - 1) == MinoNone ||
			    getPlayfieldGrid(field, x, y - 1) == MinoNone)
				queueBorderSegment((GLfloat)tx,
				                   (GLfloat)ty + 0.875f,
				                   (GLfloat)tx + 0.125f,
				                   (GLfloat)ty + 1.0f);

			// Top right
			if (getPlayfieldGrid(field, x + 1, y) == MinoNone ||
			    getPlayfieldGrid(field, x + 1, y - 1) == MinoNone ||
			    getPlayfieldGrid(field, x, y - 1) == MinoNone)
				queueBorderSegment((GLfloat)tx + 0.875f,
				                   (GLfloat)ty + 0.875f,
				                   (GLfloat)tx + 1.0f,
				                   (GLfloat)ty + 1.0f);

			// Bottom left
			if (getPlayfieldGrid(field, x - 1, y) == MinoNone ||
			    getPlayfieldGrid(field, x - 1, y + 1) == MinoNone ||
			    getPlayfieldGrid(field, x, y + 1) == MinoNone)
				queueBorderSegment((GLfloat)tx,
				                   (GLfloat)ty,
				                   (GLfloat)tx + 0.125f,
				                   (GLfloat)ty + 0.125f);

			// Bottom right
			if (getPlayfieldGrid(field, x + 1, y) == MinoNone ||
			    getPlayfieldGrid(field, x + 1, y + 1) == MinoNone ||
			    getPlayfieldGrid(field, x, y + 1) == MinoNone)
				queueBorderSegment((GLfloat)tx + 0.875f,
				                   (GLfloat)ty,
				                   (GLfloat)tx + 1.0f,
				                   (GLfloat)ty + 0.125f);
		}
	}
}

void renderBorder(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER,
	             INSTANCE_LIMIT * sizeof(struct segmentInstance),
	             NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)min(segmentQueue->count, INSTANCE_LIMIT)
	                * sizeof(struct segmentInstance), segmentQueue->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(program);
	glBindVertexArray(vao);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glUniform4f(colorAttr, 1.0f, 1.0f, 1.0f, 0.5f);
	glDrawArraysInstanced(GL_TRIANGLES, 0, countof(vertexData) / 2,
	                      (GLsizei)segmentQueue->count);

	glBindVertexArray(0);
	glUseProgram(0);

	clearDarray(segmentQueue);
}
