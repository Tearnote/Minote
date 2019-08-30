// Minote - textrender.c

#include "textrender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "render.h"
#include "util.h"
#include "log.h"
#include "queue.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ASSERT(x) assert(x)
#include "stb_image/stb_image.h"

#define FONT_PATH "ttf/Bitter-Regular_img.png"
#include "Bitter-Regular_desc.c"

#define VERTEX_LIMIT 8192

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint atlas = 0;

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
	assert(width == 512 && height == 512);

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

void queuePlayfieldText(void)
{
	char letter = 'h';
	struct textVertex *newVertex;

	GLfloat tx1 = font_Bitter_Regular_codepoint_infos[letter].atlas_x;
	GLfloat ty1 = font_Bitter_Regular_codepoint_infos[letter].atlas_y;
	GLfloat tx2 = tx1 + font_Bitter_Regular_codepoint_infos[letter].atlas_w;
	GLfloat ty2 = ty1 + font_Bitter_Regular_codepoint_infos[letter].atlas_h;
	tx1 /= 512.0f;
	ty1 /= 512.0f;
	tx2 /= 512.0f;
	ty2 /= 512.0f;

	GLfloat x1 = -4.0f;
	GLfloat y1 = 2.0f;
	GLfloat x2 = x1 + (tx2 - tx1) * 64;
	GLfloat y2 = y1 + (ty2 - ty1) * 64;

	ty1 = 1.0f - ty1;
	ty2 = 1.0f - ty2;

	newVertex = produceQueueItem(textQueue);
	newVertex->x = x1;
	newVertex->y = y1;
	newVertex->z = 1.0f;
	newVertex->tx = tx1;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = x2;
	newVertex->y = y1;
	newVertex->z = 1.0f;
	newVertex->tx = tx2;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = x2;
	newVertex->y = y2;
	newVertex->z = 1.0f;
	newVertex->tx = tx2;
	newVertex->ty = ty2;

	newVertex = produceQueueItem(textQueue);
	newVertex->x = x1;
	newVertex->y = y1;
	newVertex->z = 1.0f;
	newVertex->tx = tx1;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = x2;
	newVertex->y = y2;
	newVertex->z = 1.0f;
	newVertex->tx = tx2;
	newVertex->ty = ty2;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = x1;
	newVertex->y = y2;
	newVertex->z = 1.0f;
	newVertex->tx = tx1;
	newVertex->ty = ty2;
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