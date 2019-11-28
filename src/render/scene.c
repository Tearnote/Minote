// Minote - render/scene.c

#include "render/scene.h"

#include "glad/glad.h"

#include "util/util.h"
#include "util/log.h"
#include "render/render.h"
#include "render/ease.h"

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;
static GLint strengthAttr = -1;

#define quad(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, r, g, b, a) \
        x1, y1, z1, r, g, b, a, \
        x2, y2, z2, r, g, b, a, \
        x3, y3, z3, r, g, b, a, \
        \
        x1, y1, z1, r, g, b, a, \
        x3, y3, z3, r, g, b, a, \
        x4, y4, z4, r, g, b, a

static GLfloat vertexData[] = { // vec3 position, vec4 color
	// Level count separator
	quad(5.1f, 4.05f, -0.5f,
	     11.1f, 4.05f, -0.5f,
	     11.1f, 4.15f, -0.5f,
	     5.1f, 4.15f, -0.5f,
	     0.0f, 0.0f, 0.0f, 1.0f),

	// Clock line
	quad(-11.1f, 4.05f, -0.5f,
	     -5.1f, 4.05f, -0.5f,
	     -5.1f, 4.15f, -0.5f,
	     -11.1f, 4.15f, -0.5f,
	     0.0f, 0.0f, 0.0f, 1.0f),

	// Backplane
	quad(-5.1f, -0.1f, -1.0f,
	     5.1f, -0.1f, -1.0f,
	     5.1f, 20.1f, -1.0f,
	     -5.1f, 20.1f, -1.0f,
	     0.0f, 0.0f, 0.0f, 0.9f),

	// Bottom wall
	quad(-5.1f, -0.1f, -1.0f,
	     -5.1f, -0.1f, 0.2f,
	     5.1f, -0.1f, 0.2f,
	     5.1f, -0.1f, -1.0f,
	     0.0f, 0.0f, 0.0f, 0.95f),

	// Bottom wall highlight
	quad(-5.2f, -0.2f, 0.2f,
	     5.2f, -0.2f, 0.2f,
	     5.1f, -0.1f, 0.2f,
	     -5.1f, -0.1f, 0.2f,
	     1.0f, 1.0f, 1.0f,
	     1.0f),

	// Left wall
	quad(-5.1f, -0.1f, 0.2f,
	     -5.1f, -0.1f, -1.0f,
	     -5.1f, 20.1f, -1.0f,
	     -5.1f, 20.1f, 0.2f,
	     0.0f, 0.0f, 0.0f, 0.95f),

	// Left wall highlight
	quad(-5.2f, -0.2f, 0.2f,
	     -5.1f, -0.1f, 0.2f,
	     -5.1f, 20.1f, 0.2f,
	     -5.2f, 20.1f, 0.2f,
	     1.0f, 1.0f, 1.0f,
	     1.0f),

	// Right wall
	quad(5.1f, -0.1f, -1.0f,
	     5.1f, -0.1f, 0.2f,
	     5.1f, 20.1f, 0.2f,
	     5.1f, 20.1f, -1.0f,
	     0.0f, 0.0f, 0.0f, 0.95f),

	// Right wall highlight
	quad(5.1f, -0.1f, 0.2f,
	     5.2f, -0.2f, 0.2f,
	     5.2f, 20.1f, 0.2f,
	     5.1f, 20.1f, 0.2f,
	     1.0f, 1.0f, 1.0f,
	     1.0f),

	// Preview box
	quad(-3.0f, 20.5f, -1.0f,
	     3.0f, 20.5f, -1.0f,
	     3.0f, 23.5f, -1.0f,
	     -3.0f, 23.5f, -1.0f,
	     0.0f, 0.0f, 0.0f, 0.9f),

	// Grade box
	quad(6.0f, 14.5f, -0.5f,
	     11.0f, 14.5f, -0.5f,
	     11.0f, 19.5f, -0.5f,
	     6.0f, 19.5f, -0.5f,
	     0.0f, 0.0f, 0.0f, 0.5f)
};

static float strength = 1.0f;
static int combo = 1;

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
	strengthAttr = glGetUniformLocation(program, "strength");

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

	combo = 1;
	strength = 1.2f;
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

void updateScene(int newCombo)
{
	if (combo == newCombo)
		return;

	addEase(&strength, strength, 1.1f + 0.05f * (float)newCombo, 0.5 * SEC,
	        EaseOutQuadratic);
	combo = newCombo;
}

void renderScene(void)
{
	glUseProgram(program);
	glBindVertexArray(vao);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glUniform1f(strengthAttr, strength);
	glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 7);

	glBindVertexArray(0);
	glUseProgram(0);
}
