#include "minorender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath.h"
#include "render.h"
#include "log.h"

static GLuint program = 0;
static GLuint vertexAttrs = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;
static GLint projectionAttr = 0;
static GLfloat vertexData[] = {
	0, 0,
	0, 24,
	24, 0,
	24, 24
};
static GLfloat instanceData[] = {
	36, 24, 1, 0, 0, 1
};

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
	glGenBuffers(1, &instanceBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(instanceData), instanceData, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &vertexAttrs);
	glGenBuffers(1, &vertexBuffer);
	glBindVertexArray(vertexAttrs);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), 0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cleanupMinoRenderer(void) {
	destroyProgram(program);
}

void queueMinoPlayfield(void) {
	
}

void renderMino(void) {
	glUseProgram(program);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, &projection[0][0]);
	glBindVertexArray(vertexAttrs);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	glBindVertexArray(0);
}