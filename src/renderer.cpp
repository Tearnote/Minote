/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.hpp"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath/linmath.h"
#include "window.hpp"
#include "opengl.hpp"
#include "model.hpp"
#include "util.hpp"
#include "time.hpp"
#include "log.hpp"

using minote::log::L;

/// Basic blit function type
typedef struct ProgramBlit {
	ProgramBase base;
	TextureUnit image;
	Uniform boost;
} ProgramBlit;

/// Internal gamma correction shader
typedef struct ProgramDelinearize {
	ProgramBase base;
	TextureUnit image;
} ProgramDelinearize;

static const char* ProgramBlitVertName = "blit.vert";
static const GLchar* ProgramBlitVertSrc = (GLchar[]){
#include "blit.vert"
	'\0'};
static const char* ProgramBlitFragName = "blit.frag";
static const GLchar* ProgramBlitFragSrc = (GLchar[]){
#include "blit.frag"
	'\0'};

static const char* ProgramDelinearizeVertName = "delinearize.vert";
static const GLchar* ProgramDelinearizeVertSrc = (GLchar[]){
#include "delinearize.vert"
	'\0'};
static const char* ProgramDelinearizeFragName = "delinearize.frag";
static const GLchar* ProgramDelinearizeFragSrc = (GLchar[]){
#include "delinearize.frag"
	'\0'};

static bool initialized = false;

static Framebuffer* renderFb = nullptr;
static Texture* renderFbColor = nullptr;
static Renderbuffer* renderFbDepthStencil = nullptr;

static size2i viewportSize = {}; ///< in pixels

static Model* sync = nullptr; ///< Invisible model used to prevent frame buffering

static ProgramBlit* blit = nullptr;
static ProgramDelinearize* delinearize = nullptr;

static bool syncEnabled = true;

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
		VertexFlat syncMesh[] = {
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
		};
		sync = modelCreateFlat("sync", 3, syncMesh);
	}

	modelDraw(sync, 1, nullptr, nullptr, &IdentityMatrix);
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

	textureStorage(renderFbColor, size, GL_RGB16F);
	renderbufferStorage(renderFbDepthStencil, size, GL_DEPTH24_STENCIL8);
}

void rendererInit(void)
{
	if (initialized) return;

	// Pick up the OpenGL context
	windowContextActivate();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		L.crit("Failed to initialize OpenGL");
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
		L.crit("Failed to create the rendering framebuffer");
		exit(EXIT_FAILURE);
	}

	framebufferUse(rendererFramebuffer());

	// Create built-in shaders
	blit = programCreate(ProgramBlit,
		ProgramBlitVertName, ProgramBlitVertSrc,
		ProgramBlitFragName, ProgramBlitFragSrc);
	blit->image = programSampler(blit, "image", GL_TEXTURE0);
	blit->boost = programUniform(blit, "boost");

	delinearize = programCreate(ProgramDelinearize,
		ProgramDelinearizeVertName, ProgramDelinearizeVertSrc,
		ProgramDelinearizeFragName, ProgramDelinearizeFragSrc);
	delinearize->image = programSampler(delinearize, "image", GL_TEXTURE0);

	L.debug("Created renderer for window \"%s\"", windowGetTitle());
}

void rendererCleanup(void)
{
	if (!initialized) return;
	modelDestroy(sync);
	sync = nullptr;
	programDestroy(delinearize);
	delinearize = nullptr;
	programDestroy(blit);
	blit = nullptr;
	renderbufferDestroy(renderFbDepthStencil);
	renderFbDepthStencil = nullptr;
	textureDestroy(renderFbColor);
	renderFbColor = nullptr;
	framebufferDestroy(renderFb);
	renderFb = nullptr;
	windowContextDeactivate();
	L.debug("Destroyed renderer for window \"%s\"",
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

	glDisable(GL_BLEND);
	framebufferUse(nullptr);
	programUse(delinearize);
	textureUse(renderFbColor, delinearize->image);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	windowFlip();
	if (syncEnabled)
		rendererSync();
	glEnable(GL_BLEND);
}

void rendererBlit(Texture* t, GLfloat boost)
{
	assert(t);
	programUse(blit);
	textureUse(t, blit->image);
	glUniform1f(blit->boost, boost);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool rendererGetSync(void)
{
	return syncEnabled;
}

void rendererSetSync(bool enabled)
{
	syncEnabled = enabled;
	L.debug("%s renderer sync", enabled? "Enabling": "Disabling");
}
