/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath/linmath.h"
#include "window.h"
#include "opengl.h"
#include "model.h"
#include "util.h"
#include "time.h"
#include "log.h"

/// Basic blit function type
typedef struct ProgramBlit {
	ProgramBase base;
	TextureUnit image;
	Uniform boost;
} ProgramBlit;

static const char* ProgramBlitVertName = u8"blit.vert";
static const GLchar* ProgramBlitVertSrc = (GLchar[]){
#include "blit.vert"
	'\0'};
static const char* ProgramBlitFragName = u8"blit.frag";
static const GLchar* ProgramBlitFragSrc = (GLchar[]){
#include "blit.frag"
	'\0'};

static bool initialized = false;

static Framebuffer* renderFb = null;
static Texture* renderFbColor = null;
static Renderbuffer* renderFbDepthStencil = null;

static size2i viewportSize = {0}; ///< in pixels

static Model* sync = null; ///< Invisible model used to prevent frame buffering

static ProgramBlit* blit = null;

/**
 * Prevent the driver from buffering commands. Call this after windowFlip()
 * to minimize video latency.
 * @see https://danluu.com/latency-mitigation/
 */
static void rendererSync(void)
{
	assert(initialized);

	// Create sync model if it doesn't exist yet
	if (!sync) {
		sync = modelCreateFlat(u8"sync", 3, (VertexFlat[]){
			{
				.pos = {0.0f, 0.0f, 0.0f},
				.color = Color4Clear
			},
			{
				.pos = {1.0f, 0.0f, 0.0f},
				.color = Color4Clear
			},
			{
				.pos = {0.0f, 1.0f, 0.0f},
				.color = Color4Clear
			}
		});
	}

	modelDraw(sync, 1, null, null, &IdentityMatrix);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, secToNsec(0.1));
}

/**
 * Resize the rendering viewport, preferably to window size. Recreates the
 * matrices and framebuffers as needed.
 * @param size New viewport size in pixels
 */
static void rendererResize(size2i size)
{
	assert(initialized);
	assert(size.x > 0);
	assert(size.y > 0);
	if (viewportSize.x == size.x && viewportSize.y == size.y) return;
	viewportSize.x = size.x;
	viewportSize.y = size.y;

	textureStorage(renderFbColor, size, GL_RGBA16F);
	renderbufferStorage(renderFbDepthStencil, size, GL_DEPTH24_STENCIL8);
}

void rendererInit(void)
{
	if (initialized) return;

	// Pick up the OpenGL context
	windowContextActivate();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit(applog, u8"Failed to initialize OpenGL");
		exit(EXIT_FAILURE);
	}
	initialized = true;

	// Set up global OpenGL state
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);

	// Create framebuffers
	renderFb = framebufferCreate();
	renderFbColor = textureCreate();
	renderFbDepthStencil = renderbufferCreate();

	// Set up matrices and framebuffer textures
	rendererResize(windowGetSize());

	// Put framebuffers together
	framebufferTexture(renderFb, renderFbColor, GL_COLOR_ATTACHMENT0);
	framebufferRenderbuffer(renderFb, renderFbDepthStencil,
		GL_DEPTH_STENCIL_ATTACHMENT);
	if (!framebufferCheck(renderFb)) {
		logCrit(applog, u8"Failed to create the rendering framebuffer");
		exit(EXIT_FAILURE);
	}

	framebufferUse(rendererFramebuffer());

	// Create built-in shaders
	blit = programCreate(ProgramBlit,
		ProgramBlitVertName, ProgramBlitVertSrc,
		ProgramBlitFragName, ProgramBlitFragSrc);
	blit->image = programSampler(blit, u8"image", GL_TEXTURE0);
	blit->boost = programUniform(blit, u8"boost");

	logDebug(applog, u8"Created renderer for window \"%s\"", windowGetTitle());
}

void rendererCleanup(void)
{
	if (!initialized) return;
	modelDestroy(sync);
	sync = null;
	programDestroy(blit);
	blit = null;
	renderbufferDestroy(renderFbDepthStencil);
	renderFbDepthStencil = null;
	textureDestroy(renderFbColor);
	renderFbColor = null;
	framebufferDestroy(renderFb);
	renderFb = null;
	windowContextDeactivate();
	logDebug(applog, u8"Destroyed renderer for window \"%s\"",
		windowGetTitle());
	initialized = false;
}

Framebuffer* rendererFramebuffer(void)
{
	return renderFb;
}

Texture* rendererTexture(void)
{
	return renderFbColor;
}

void rendererFrameBegin(void)
{
	assert(initialized);

	rendererResize(windowGetSize());
	framebufferUse(renderFb);
}

void rendererFrameEnd(void)
{
	assert(initialized);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, viewportSize.x, viewportSize.y,
		0, 0, viewportSize.x, viewportSize.y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Present the frame
	windowFlip();
	rendererSync();
}

void rendererDepthOnlyBegin(void)
{
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void rendererDepthOnlyEnd(void)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void rendererBlit(Texture* t, GLfloat boost)
{
	programUse(blit);
	textureUse(t, blit->image);
	glUniform1f(blit->boost, boost);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}
