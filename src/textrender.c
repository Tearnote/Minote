// Minote - textrender.c

#include "textrender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "util.h"
#define STBI_ASSERT(x) assert(x)
#include "stb_image/stb_image.h"
#include "linmath/linmath.h"

#include "render.h"
#include "util.h"
#include "log.h"
#include "queue.h"

#define FONT_PATH "ttf/Bitter-Regular_img.png"
#include "Bitter-Regular_desc.c"

#define VERTEX_LIMIT 8192

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint atlas = 0;
static int atlasSize = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

struct textVertex {
	GLfloat x, y, z;
	GLfloat tx, ty;
};
static queue *textQueue = NULL;

void initTextRenderer(void)
{
	textQueue = createQueue(sizeof(struct textVertex));

	unsigned char *atlasData = NULL;
	int width;
	int height;
	int channels;
	atlasData = stbi_load(FONT_PATH, &width, &height, &channels, 4);
	if (!atlasData) {
		logError("Failed to load %s: %s", FONT_PATH,
		         stbi_failure_reason());
		return;
	}
	assert(width == height);
	atlasSize = width;

	glGenTextures(1, &atlas);
	glBindTexture(GL_TEXTURE_2D, atlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, atlasData);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(atlasData);

	const GLchar vertSrc[] = {
#include "text.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "text.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize text renderer");
	cameraAttr = glGetUniformLocation(program, "camera");
	projectionAttr = glGetUniformLocation(program, "projection");

	glGenBuffers(1, &vertexBuffer);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5,
	                      (GLvoid *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5,
	                      (GLvoid *)(sizeof(GLfloat) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void cleanupTextRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;
	glDeleteTextures(1, &atlas);
	atlas = 0;
}

static void
queueGlyph(unsigned char glyph, vec3 position, quat orientation, GLfloat size)
{
	vec3 bottomLeft = {};
	vec3 bottomRight = {};
	vec3 topLeft = {};
	vec3 topRight = {};

	GLfloat tx1 =
		(GLfloat)font_Bitter_Regular_codepoint_infos[glyph].atlas_x;
	GLfloat ty1 =
		(GLfloat)font_Bitter_Regular_codepoint_infos[glyph].atlas_y;
	GLfloat tx2 = tx1 + (GLfloat)font_Bitter_Regular_codepoint_infos[glyph]
		.atlas_w;
	GLfloat ty2 = ty1 + (GLfloat)font_Bitter_Regular_codepoint_infos[glyph]
		.atlas_h;
	GLfloat w = (tx2 - tx1)
	            / (GLfloat)font_Bitter_Regular_information.max_height
	            * size;
	GLfloat h = (ty2 - ty1)
	            / (GLfloat)font_Bitter_Regular_information.max_height
	            * size;
	tx1 /= (GLfloat)atlasSize;
	ty1 /= (GLfloat)atlasSize;
	tx2 /= (GLfloat)atlasSize;
	ty2 /= (GLfloat)atlasSize;
	ty1 = 1.0f - ty1;
	ty2 = 1.0f - ty2;

	bottomLeft[0] = position[0];
	bottomLeft[1] = position[1];
	bottomLeft[2] = position[2];

	bottomRight[0] = position[0] + w;
	bottomRight[1] = position[1];
	bottomRight[2] = position[2];

	topLeft[0] = position[0];
	topLeft[1] = position[1] + h;
	topLeft[2] = position[2];

	topRight[0] = position[0] + w;
	topRight[1] = position[1] + h;
	topRight[2] = position[2];

	vec3_sub(bottomLeft, bottomLeft, position);
	vec3_sub(bottomRight, bottomRight, position);
	vec3_sub(topLeft, topLeft, position);
	vec3_sub(topRight, topRight, position);
	quat_mul_vec3(bottomLeft, orientation, bottomLeft);
	quat_mul_vec3(bottomRight, orientation, bottomRight);
	quat_mul_vec3(topLeft, orientation, topLeft);
	quat_mul_vec3(topRight, orientation, topRight);
	vec3_add(bottomLeft, bottomLeft, position);
	vec3_add(bottomRight, bottomRight, position);
	vec3_add(topLeft, topLeft, position);
	vec3_add(topRight, topRight, position);

	struct textVertex *newVertex;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = bottomLeft[0];
	newVertex->y = bottomLeft[1];
	newVertex->z = bottomLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = bottomRight[0];
	newVertex->y = bottomRight[1];
	newVertex->z = bottomRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = topRight[0];
	newVertex->y = topRight[1];
	newVertex->z = topRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty2;

	newVertex = produceQueueItem(textQueue);
	newVertex->x = bottomLeft[0];
	newVertex->y = bottomLeft[1];
	newVertex->z = bottomLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = topRight[0];
	newVertex->y = topRight[1];
	newVertex->z = topRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty2;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = topLeft[0];
	newVertex->y = topLeft[1];
	newVertex->z = topLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty2;
}

void queuePlayfieldText(void)
{
	vec3 position = { 4.6f, 0.0f, 2.0f };
	quat orientation;
	vec3 axis = { 1.0f, 0.0f, 0.0f };
	vec3_norm(axis, axis);
	quat_rotate(orientation, radf(-90.0f), axis);
	queueGlyph('h', position, orientation, 5.0f);
}

void renderText(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct textVertex) * VERTEX_LIMIT,
	             NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)MIN(textQueue->count, VERTEX_LIMIT)
	                * sizeof(struct textVertex), textQueue->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(program);
	glBindVertexArray(vao);
	glBindTexture(GL_TEXTURE_2D, atlas);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)textQueue->count);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	clearQueue(textQueue);
}