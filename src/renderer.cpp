/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.hpp"

#include <functional>
#include <string.h>
#include <stdlib.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath/linmath.h"
#include "sys/window.hpp"
#include "sys/opengl.hpp"
#include "model.hpp"
#include "base/util.hpp"
#include "base/time.hpp"
#include "base/log.hpp"

using namespace minote;

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

static Window* window = nullptr;

static Framebuffer renderFb;
static Texture renderFbColor;
static Renderbuffer renderFbDepthStencil;

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
	ASSERT(initialized);

	// Create sync model if it doesn't exist yet
	if (!sync) {
		VertexFlat syncMesh[] = {
			{
				.pos = {0.0f, 0.0f, 0.0f},
				.color = Clear4
			},
			{
				.pos = {1.0f, 0.0f, 0.0f},
				.color = Clear4
			},
			{
				.pos = {0.0f, 1.0f, 0.0f},
				.color = Clear4
			}
		};
		sync = modelCreateFlat("sync", 3, syncMesh);
	}

	modelDraw(sync, 1, nullptr, nullptr, &IdentityMatrix);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, milliseconds(100));
}

/**
 * Resize the rendering viewport, preferably to window size. Recreates the
 * matrices and framebuffers as needed.
 * @param size New viewport size in pixels
 */
static void rendererResize(size2i size)
{
	ASSERT(initialized);
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	if (viewportSize.x == size.x && viewportSize.y == size.y) return;
	viewportSize.x = size.x;
	viewportSize.y = size.y;

	renderFbColor.resize(size);
	renderFbDepthStencil.resize(size);
}

#ifndef NDEBUG
static void APIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint,
	GLenum severity, GLsizei, const GLchar* message, const void*)
{
	const char* const sourceStr = [=] {
		switch (source) {
		case GL_DEBUG_SOURCE_API:
			return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			return "window";
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			return "shader compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			return "third party";
		case GL_DEBUG_SOURCE_APPLICATION:
			return "application";
		case GL_DEBUG_SOURCE_OTHER:
			return "other";
		default:
			return "unknown";
		}
	}();
	const char* const typeStr = [=] {
		switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			return "deprecated behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			return "undefined behavior";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "portability";
		case GL_DEBUG_TYPE_PERFORMANCE:
			return "performance";
		case GL_DEBUG_TYPE_MARKER:
			return "marker";
		case GL_DEBUG_TYPE_PUSH_GROUP:
			return "pushed group";
		case GL_DEBUG_TYPE_POP_GROUP:
			return "popped group";
		case GL_DEBUG_TYPE_OTHER:
			return "other";
		default:
			return "unknown";
		}
	}();
	const auto logFunc = [=] {
		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			return &Log::error;
		case GL_DEBUG_SEVERITY_MEDIUM:
			return &Log::warn;
		case GL_DEBUG_SEVERITY_LOW:
			return &Log::info;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return &Log::debug;
		default:
			L.warn("Unknown OpenGL message severity %d", severity);
			return &Log::warn;
		}
	}();

	std::invoke(logFunc, L, "OpenGL %s message from the %s: %s",
		typeStr, sourceStr, message);
}
#endif //NDEBUG

void rendererInit(Window& w)
{
	if (initialized) return;

	window = &w;

	// Pick up the OpenGL context
	window->activateContext();
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
#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Our log handling is fast
	glDebugMessageCallback(debugMessageCallback, nullptr);
#endif //NDEBUG

	// Create framebuffers
	renderFb.create("renderFb");
	renderFbColor.create("renderFbColor", window->size, PixelFormat::RGB_f16);
	renderFbDepthStencil.create("renderFbDepthStencil", window->size, PixelFormat::DepthStencil);

	// Set up matrices and framebuffer textures
	rendererResize(window->size);

	// Put framebuffers together
	renderFb.attach(renderFbColor, Attachment::Color0);
	renderFb.attach(renderFbDepthStencil, Attachment::DepthStencil);

	rendererFramebuffer().bind();

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

	L.debug("Created renderer for window \"%s\"", window->title);
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
	renderFbDepthStencil.destroy();
	renderFbColor.destroy();
	renderFb.destroy();
	window->deactivateContext();
	L.debug("Destroyed renderer for window \"%s\"", window->title);
	initialized = false;
}

Framebuffer& rendererFramebuffer(void)
{
	return renderFb;
}

Texture& rendererTexture(void)
{
	return renderFbColor;
}

void rendererFrameBegin(void)
{
	ASSERT(initialized);

	rendererResize(window->size);
	renderFb.bind();
}

void rendererFrameEnd(void)
{
	ASSERT(initialized);

	glDisable(GL_BLEND);
	Framebuffer::unbind();
	programUse(delinearize);
	renderFbColor.bind(delinearize->image);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	window->flip();
	if (syncEnabled)
		rendererSync();
	glEnable(GL_BLEND);
}

void rendererBlit(Texture& t, GLfloat boost)
{
	programUse(blit);
	t.bind(blit->image);
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
