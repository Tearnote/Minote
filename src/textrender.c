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

#define INSTANCE_LIMIT 256

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;
static GLuint atlas = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

static GLfloat vertexData[] = { // vec3 position
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,

	0.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0, 0.0f
};

struct textInstance {
	GLfloat x, y;
	GLfloat w, h;
	GLfloat tx, ty;
	GLfloat tw, th;
};
static queue *textQueue = NULL;

void initTextRenderer(void)
{
	textQueue = createQueue(sizeof(struct textInstance));

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3,
	                      (GLvoid *)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
	                      (GLvoid *)0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
	                      (GLvoid *)(sizeof(GLfloat) * 2));
	glVertexAttribDivisor(2, 1);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
	                      (GLvoid *)(sizeof(GLfloat) * 4));
	glVertexAttribDivisor(3, 1);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
	                      (GLvoid *)(sizeof(GLfloat) * 6));
	glVertexAttribDivisor(4, 1);
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
	struct textInstance *newInstance = produceQueueItem(textQueue);
	newInstance->x = -4.0f;
	newInstance->y = 1.0f;
	newInstance->w =
		(GLfloat)font_Bitter_Regular_codepoint_infos[letter].atlas_w
		/ 4.0f;
	newInstance->h =
		(GLfloat)font_Bitter_Regular_codepoint_infos[letter].atlas_h
		/ 4.0f;
	newInstance->tx =
		(GLfloat)font_Bitter_Regular_codepoint_infos[letter].atlas_x
		/ 512.0f;
	newInstance->ty =
		(GLfloat)font_Bitter_Regular_codepoint_infos[letter].atlas_y
		/ 512.0f;
	newInstance->tw =
		(GLfloat)font_Bitter_Regular_codepoint_infos[letter].atlas_w
		/ 512.0f;
	newInstance->th =
		(GLfloat)font_Bitter_Regular_codepoint_infos[letter].atlas_h
		/ 512.0f;
}

void renderText(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER,
	             INSTANCE_LIMIT * sizeof(struct textInstance),
	             NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)MIN(textQueue->count, INSTANCE_LIMIT)
	                * sizeof(struct textInstance), textQueue->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(program);
	glBindVertexArray(vao);
	glBindTexture(GL_TEXTURE_2D, atlas);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, COUNT_OF(vertexData) / 3,
	                      (GLsizei)textQueue->count);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	clearQueue(textQueue);
}