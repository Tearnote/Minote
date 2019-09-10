// Minote - scenerender.c

#include "scenerender.h"

#include <math.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "render.h"
#include "log.h"
#include "util.h"

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

static GLfloat vertexData[] = { // vec3 position, vec4 color
	// Level count separator
	5.1f, 4.05f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	11.1f, 4.05f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	11.1f, 4.15f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,

	11.1f, 4.15f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	5.1f, 4.15f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	5.1f, 4.05f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,

	// Clock line
	-5.1f, 4.05f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	-11.1f, 4.15f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	-11.1f, 4.05f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,

	-11.1f, 4.15f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	-5.1f, 4.05f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
	-5.1f, 4.15f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,

	// Backplane
	-5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,

	-5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	-5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,

	// Bottom wall
	-5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,
	-5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,

	-5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,

	// Bottom wall highlight
	-5.1f, -0.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	-5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.1f, -0.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,

	-5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.1f, -0.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,

	// Left wall
	-5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,
	-5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,
	-5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,

	-5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	-5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,
	-5.1f, 20.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,

	// Left wall highlight
	-5.1f, -0.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	-5.1f, 20.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	-5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,

	-5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	-5.1f, 20.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	-5.2f, 20.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,

	// Right wall
	5.1f, -0.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,

	5.1f, -0.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, 20.1f, 0.2f, 0.0f, 0.0f, 0.0f, 0.95f,
	5.1f, 20.1f, -1.0f, 0.0f, 0.0f, 0.0f, 0.95f,

	// Right wall highlight
	5.1f, -0.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.1f, 20.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,

	5.2f, -0.2f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.2f, 20.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,
	5.1f, 20.1f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f,

	// Preview box
	-3.0f, 20.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	3.0f, 20.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	3.0f, 23.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,

	3.0f, 23.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	-3.0f, 23.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,
	-3.0f, 20.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.9f,

	// Grade box
	6.0f, 14.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,
	11.0f, 14.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,
	11.0f, 19.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,

	11.0f, 19.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,
	6.0f, 19.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,
	6.0f, 14.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f
};

void initSceneRenderer(void)
{
	const GLchar vertSrc[] = {
#include "scene.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "scene.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize scene renderer");
	cameraAttr = glGetUniformLocation(program, "camera");
	projectionAttr = glGetUniformLocation(program, "projection");

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7,
	                      (GLvoid *)0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7,
	                      (GLvoid *)(sizeof(GLfloat) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void cleanupSceneRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;
}

void renderScene(void)
{
	glUseProgram(program);
	glBindVertexArray(vao);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 7);

	glBindVertexArray(0);
	glUseProgram(0);
}
