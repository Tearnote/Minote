// Minote - textrender.h

#include "textrender.h"

#include <stdlib.h>
#include <stdio.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "util.h"
#include "render.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ASSERT(x) assert(x)
#include "stb/stb_image.h"

#include "log.h"

#define FILENAME_TEMPLATE "ttf/B612Mono-Regular-xxx.png"
#define FILENAME_PATTERN "ttf/B612Mono-Regular-%d.png"

#define GLYPH_SIZE 64
#define GLYPH_LAYER (GLYPH_SIZE * GLYPH_SIZE * 3)
#define GLYPH_FIRST 32
#define GLYPH_LAST 126
#define GLYPH_COUNT (GLYPH_LAST - GLYPH_FIRST + 1)

static GLuint font = 0;
static GLuint program = 0;
static GLuint vertexAttrs = 0;
static GLuint vertexBuffer = 0;
static GLint projectionAttr = 0;
static GLfloat vertexData[] = {
	0, 0,
	0, GLYPH_SIZE,
	GLYPH_SIZE, 0,
	GLYPH_SIZE, GLYPH_SIZE
};

void initTextRenderer(void)
{
	unsigned char *glyphs = allocate(GLYPH_LAYER * GLYPH_COUNT);
	for (int i = GLYPH_FIRST; i <= GLYPH_LAST; i++) {
		char filename[] = FILENAME_TEMPLATE;
		sprintf(filename, FILENAME_PATTERN, i);
		int x;
		int y;
		int channels;
		unsigned char
			*glyph = stbi_load(filename, &x, &y, &channels, 3);
		if (!glyph) {
			logError("Failed to open image %s: %s", filename,
			         stbi_failure_reason());
			free(glyphs);
			glyphs = NULL;
			return;
		}
		memcpy(glyphs + GLYPH_LAYER * (i - GLYPH_FIRST), glyph,
		       GLYPH_LAYER);
		stbi_image_free(glyph);
	}

	glGenTextures(1, &font);
	glBindTexture(GL_TEXTURE_2D_ARRAY, font);
	glTexImage3D(GL_TEXTURE_2D_ARRAY,
	             0,                 // mipmap level
	             GL_RGB8,           // gpu texel format
	             GLYPH_SIZE,        // width
	             GLYPH_SIZE,        // height
	             GLYPH_COUNT,       // depth
	             0,                 // border
	             GL_RGB,            // cpu pixel format
	             GL_UNSIGNED_BYTE,  // cpu pixel coord type
	             glyphs);           // pixel data
	glTexParameteri(GL_TEXTURE_2D_ARRAY,
	                GL_TEXTURE_MIN_FILTER,
	                GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,
	                GL_TEXTURE_MAG_FILTER,
	                GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,
	                GL_TEXTURE_WRAP_S,
	                GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,
	                GL_TEXTURE_WRAP_T,
	                GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	free(glyphs);

	const GLchar vertSrc[] = {
#include "msdf.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "msdf.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize text renderer");
	projectionAttr = glGetUniformLocation(program, "projection");

	glGenVertexArrays(1, &vertexAttrs);
	glGenBuffers(1, &vertexBuffer);
	glBindVertexArray(vertexAttrs);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData),
	             vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cleanupTextRenderer(void)
{
	glDeleteTextures(1, &font);
	font = 0;
	destroyProgram(program);
	program = 0;
}

void renderText(void)
{
	glUseProgram(program);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, &projection[0][0]);
	glBindVertexArray(vertexAttrs);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
