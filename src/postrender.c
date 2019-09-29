// Minote - postrender.c

#include "postrender.h"

#include <stdlib.h>

#include "glad/glad.h"

#include "AHEasing/easing.h"

#include "render.h"
#include "util.h"
#include "log.h"
#include "window.h"
#include "timer.h"

#define VIGNETTE_BASE 0.4f
#define VIGNETTE_MAX 0.46f
#define VIGNETTE_PULSE (0.9 * SEC)

static GLuint renderFbo = 0;
static GLuint renderFboColor = 0;
static GLuint renderFboDepth = 0;

static GLuint resolveFbo = 0;
static GLuint resolveFboColor = 0;

static GLuint bloomFbo = 0;
static GLuint bloomFboColor = 0;
static GLuint bloom2Fbo = 0;
static GLuint bloom2FboColor = 0;

static GLuint thresholdProgram = 0;
static GLint thresholdAttr = -1;
static GLuint blurProgram = 0;
static GLint stepAttr = -1;
static GLuint composeProgram = 0;
static GLint screenAttr = -1;
static GLint bloomAttr = -1;
static GLint bloomStrengthAttr = -1;
static GLuint vignetteProgram = 0;
static GLint falloffAttr = -1;
static GLint aspectAttr = -1;

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
static int fboHeight = DEFAULT_HEIGHT;
static int bloomWidth = DEFAULT_WIDTH;
static int bloomHeight = DEFAULT_HEIGHT;

static int blurKernel[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10 };

static float falloff = VIGNETTE_BASE;

static nsec vignettePulseStart = -1;
static float vignettePulseSpeed = 1.0f;

void initPostRenderer(void)
{
	// Create the shader programs
	const GLchar thresholdVertSrc[] = {
#include "threshold.vert"
		, 0x00 };
	const GLchar thresholdFragSrc[] = {
#include "threshold.frag"
		, 0x00 };
	thresholdProgram = createProgram(thresholdVertSrc, thresholdFragSrc);
	if (thresholdProgram == 0)
		logError("Failed to initialize post renderer");
	thresholdAttr = glGetUniformLocation(thresholdProgram, "threshold");

	const GLchar blurVertSrc[] = {
#include "blur.vert"
		, 0x00 };
	const GLchar blurFragSrc[] = {
#include "blur.frag"
		, 0x00 };
	blurProgram = createProgram(blurVertSrc, blurFragSrc);
	if (blurProgram == 0)
		logError("Failed to initialize post renderer");
	stepAttr = glGetUniformLocation(blurProgram, "step");

	const GLchar composeVertSrc[] = {
#include "compose.vert"
		, 0x00 };
	const GLchar composeFragSrc[] = {
#include "compose.frag"
		, 0x00 };
	composeProgram = createProgram(composeVertSrc, composeFragSrc);
	if (composeProgram == 0)
		logError("Failed to initialize post renderer");
	screenAttr = glGetUniformLocation(composeProgram, "screen");
	bloomAttr = glGetUniformLocation(composeProgram, "bloom");
	bloomStrengthAttr =
		glGetUniformLocation(composeProgram, "bloomStrength");

	const GLchar vignetteVertSrc[] = {
#include "vignette.vert"
		, 0x00 };
	const GLchar vignetteFragSrc[] = {
#include "vignette.frag"
		, 0x00 };
	vignetteProgram = createProgram(vignetteVertSrc, vignetteFragSrc);
	if (vignetteProgram == 0)
		logError("Failed to initialize post renderer");
	falloffAttr = glGetUniformLocation(vignetteProgram, "falloff");
	aspectAttr = glGetUniformLocation(vignetteProgram, "aspect");

	// Create VAO
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

	// Create the initial framebuffer
	glGenFramebuffers(1, &renderFbo);

	glGenTextures(1, &renderFboColor);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFboColor);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER,
	                GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER,
	                GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glGenRenderbuffers(1, &renderFboDepth);

	// Create the resolve framebuffer
	glGenFramebuffers(1, &resolveFbo);

	glGenTextures(1, &resolveFboColor);
	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create the bloom framebuffers
	glGenFramebuffers(1, &bloomFbo);

	glGenTextures(1, &bloomFboColor);
	glBindTexture(GL_TEXTURE_2D, bloomFboColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &bloom2Fbo);

	glGenTextures(1, &bloom2FboColor);
	glBindTexture(GL_TEXTURE_2D, bloom2FboColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Initialize the framebuffer attachments
	resizePostRender(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	// Bind the framebuffers
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

	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D, bloomFboColor, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
	    != GL_FRAMEBUFFER_COMPLETE) {
		logCrit("Failed to initialize bloom framebuffer");
		exit(EXIT_FAILURE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, bloom2Fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D, bloom2FboColor, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
	    != GL_FRAMEBUFFER_COMPLETE) {
		logCrit("Failed to initialize bloom2 framebuffer");
		exit(EXIT_FAILURE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizePostRender(int width, int height)
{
	bloomHeight = min(720, height);
	bloomWidth = (float)bloomHeight * ((float)width / (float)height);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFboColor);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8,
	                        width, height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, renderFboDepth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT,
	                                 width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA,
	             GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, bloomFboColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, bloomWidth, bloomHeight, 0,
	             GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, bloom2FboColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, bloomWidth, bloomHeight, 0,
	             GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	fboWidth = width;
	fboHeight = height;
}

void cleanupPostRenderer(void)
{
	glDeleteTextures(1, &bloom2FboColor);
	bloom2FboColor = 0;
	glDeleteFramebuffers(1, &bloom2Fbo);
	bloom2Fbo = 0;
	glDeleteTextures(1, &bloomFboColor);
	bloomFboColor = 0;
	glDeleteFramebuffers(1, &bloomFbo);
	bloomFbo = 0;
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

void pulseVignette(float speed)
{
	vignettePulseStart = getTime();
	vignettePulseSpeed = speed;
}

static void calculateVignette(void)
{
	nsec length =
		(double)VIGNETTE_PULSE * (double)(1.0f / vignettePulseSpeed);
	if (vignettePulseStart == -1) {
		falloff = VIGNETTE_BASE;
		return;
	}
	nsec time = getTime();
	if (time >= vignettePulseStart + length) {
		falloff = VIGNETTE_BASE;
		return;
	}

	// We are in the middle of a pulse
	nsec elapsed = time - vignettePulseStart;
	nsec thirdLength = length / 3;
	if (elapsed < thirdLength) { // We are in the first half
		float progress =
			(double)elapsed / (double)(thirdLength);
		progress = SineEaseOut(progress);
		falloff = VIGNETTE_BASE
		          + (VIGNETTE_MAX - VIGNETTE_BASE) * progress;
	} else { // They had us in the second half
		float progress = (double)(elapsed - thirdLength)
		                 / (double)(thirdLength * 2);
		progress = 1.0f - progress;
		//progress = SineEaseOut(progress);
		falloff = VIGNETTE_BASE
		          + (VIGNETTE_MAX - VIGNETTE_BASE) * progress;
	}
}

void renderPostStart(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, renderFbo);
}

void renderPostEnd(void)
{
	// Resolve the MSAA image
	glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
	glBlitFramebuffer(0, 0, fboWidth, fboHeight, 0, 0, fboWidth, fboHeight,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Draw the threshold
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo);
	glViewport(0, 0, bloomWidth, bloomHeight);

	glUseProgram(thresholdProgram);
	glBindVertexArray(vao);
	glDisable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glUniform1f(thresholdAttr, 0.7f);
	glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 4);

	// Draw blur
	glUseProgram(blurProgram);

	for (int i = 0; i < countof(blurKernel); i++) {
		GLuint fb = (i % 2 == 0) ? (bloom2Fbo) : (bloomFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);

		GLuint tx = (i % 2 == 0) ? (bloomFboColor) : (bloom2FboColor);
		glBindTexture(GL_TEXTURE_2D, tx);
		glUniform1i(stepAttr, blurKernel[i]);
		glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 4);
	}

	// Draw bloom
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, fboWidth, fboHeight);

	glUseProgram(composeProgram);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, resolveFboColor);
	glUniform1i(screenAttr, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D,
	              (countof(blurKernel) % 2 == 0) ? (bloomFboColor)
	                                             : (bloom2FboColor));
	glUniform1i(bloomAttr, 1);
	glUniform1f(bloomStrengthAttr, 0.4f);
	glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 4);

	glActiveTexture(GL_TEXTURE0);

	// Draw vignette
	glUseProgram(vignetteProgram);

	calculateVignette();
	glUniform1f(falloffAttr, falloff);
	glUniform1f(aspectAttr, (float)fboWidth / (float)fboHeight);
	glDrawArrays(GL_TRIANGLES, 0, countof(vertexData) / 4);

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glUseProgram(0);
}