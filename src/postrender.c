// Minote - postrender.c

#include "postrender.h"

#include <stdlib.h>

#include "glad/glad.h"

#include "render.h"
#include "util.h"
#include "log.h"
#include "window.h"

static GLuint renderFbo = 0;
static GLuint renderFboColor = 0;
static GLuint renderFboDepth = 0;

static GLuint resolveFbo = 0;
static GLuint resolveFboColor = 0;

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;

float vertexData[] = { // vec2 position, vec2 texcoords
	-1.0f, 1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 1.0f, 0.0f,

	-1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};

static int fboWidth = DEFAULT_WIDTH;
static int fboHeight = DEFAULT_WIDTH;

void initPostRenderer(void)
{
	const GLchar vertSrc[] = {
#include "post.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "post.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize post renderer");

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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4,
	                      (GLvoid *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4,
	                      (GLvoid *)(sizeof(GLfloat) * 2));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glGenFramebuffers(1, &renderFbo);

	glGenTextures(1, &renderFboColor);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFboColor);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER,
	                GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER,
	                GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glGenRenderbuffers(1, &renderFboDepth);

	glGenFramebuffers(1, &resolveFbo);

	glGenTextures(1, &resolveFboColor);
	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	resizePostRender(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	glBindFramebuffer(GL_FRAMEBUFFER, renderFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D_MULTISAMPLE, renderFboColor, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	                          GL_RENDERBUFFER, renderFboDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
	    != GL_FRAMEBUFFER_COMPLETE) {
		logCrit("Failed to initialize render framebuffer");
		exit(EXIT_FAILURE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D, resolveFboColor, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
	    != GL_FRAMEBUFFER_COMPLETE) {
		logCrit("Failed to initialize resolve framebuffer");
		exit(EXIT_FAILURE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizePostRender(int width, int height)
{
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFboColor);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA,
	                        width, height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, renderFboDepth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT,
	                                 width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	fboWidth = width;
	fboHeight = height;
}

void cleanupPostRenderer(void)
{
	glDeleteTextures(1, &resolveFboColor);
	resolveFboColor = 0;
	glDeleteFramebuffers(1, &resolveFbo);
	resolveFbo = 0;
	glDeleteTextures(1, &renderFboDepth);
	renderFboDepth = 0;
	glDeleteTextures(1, &renderFboColor);
	renderFboColor = 0;
	glDeleteFramebuffers(1, &renderFbo);
	renderFbo = 0;
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
}

void renderPostStart(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, renderFbo);
}

void renderPostEnd(void)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
	glBlitFramebuffer(0, 0, fboWidth, fboHeight, 0, 0, fboWidth, fboHeight,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(program);
	glBindVertexArray(vao);
	glDisable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 4);

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glUseProgram(0);
}