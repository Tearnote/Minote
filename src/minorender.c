#include "minorender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath.h"
#include "render.h"
#include "log.h"

static GLuint minoProgram = 0;
static GLuint minoVAO = 0;
static GLuint minoVBO = 0;
static GLuint minoInstanceVBO = 0;
static GLint minoUniform = 0;
static GLfloat minoVertices[] = {
	0, 0,
	0, 24,
	24, 0,
	24, 24
};
static GLfloat minoInstance[] = {
	36, 24, 1, 0, 0, 1
};

void initMinoRenderer(void) {
	const GLchar minoVertSrc[] = {
		#include "mino.vert"
	, 0x00};
	const GLchar minoFragSrc[] = {
		#include "mino.frag"
	, 0x00};
	minoProgram = createProgram(minoVertSrc, minoFragSrc);
	if(minoProgram == 0)
		logError("Failed to initialize mino renderer");
	minoUniform = glGetUniformLocation(minoProgram, "projection");
	glGenBuffers(1, &minoInstanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, minoInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(minoInstance), minoInstance, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &minoVAO);
	glGenBuffers(1, &minoVBO);
	glBindVertexArray(minoVAO);
	glBindBuffer(GL_ARRAY_BUFFER, minoVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(minoVertices), minoVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, minoInstanceVBO);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), 0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cleanupMinoRenderer(void) {
	destroyProgram(minoProgram);
}

void queueMinoPlayfield(void) {
	
}

void renderMino(void) {
	glUseProgram(minoProgram);
	glUniformMatrix4fv(minoUniform, 1, GL_FALSE, &projection[0][0]);
	glBindVertexArray(minoVAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	glBindVertexArray(0);
}