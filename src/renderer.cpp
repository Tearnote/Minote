/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.hpp"

#include <functional>
#include <string.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "sys/window.hpp"
#include "sys/opengl.hpp"
#include "model.hpp"
#include "base/util.hpp"
#include "base/time.hpp"
#include "base/log.hpp"

using namespace minote;

/// Basic blit function type
struct Blit : Shader {

	Sampler<Texture> image;
	Uniform<float> boost;

	void setLocations() override
	{
		image.setLocation(*this, "image");
		boost.setLocation(*this, "boost");
	}
};

/// Internal gamma correction shader
struct Delinearize : Shader {

	Sampler<Texture> image;

	void setLocations() override
	{
		image.setLocation(*this, "image");
	}

};

static const GLchar* BlitVertSrc = (GLchar[]){
#include "blit.vert"
	'\0'};
static const GLchar* BlitFragSrc = (GLchar[]){
#include "blit.frag"
	'\0'};

static const GLchar* DelinearizeVertSrc = (GLchar[]){
#include "delinearize.vert"
	'\0'};
static const GLchar* DelinearizeFragSrc = (GLchar[]){
#include "delinearize.frag"
	'\0'};

static bool initialized = false;

static Window* window = nullptr;

Framebuffer renderFb;
Texture<PixelFmt::RGBA_f16> renderFbColor;
Renderbuffer<PixelFmt::DepthStencil> renderFbDepthStencil;

static uvec2 viewportSize = {}; ///< in pixels

static Model* sync = nullptr; ///< Invisible model used to prevent frame buffering

static Blit blit;
static Delinearize delinearize;

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
				.color = {1.0f, 1.0f, 1.0f, 0.0f}
			},
			{
				.pos = {1.0f, 0.0f, 0.0f},
				.color = {1.0f, 1.0f, 1.0f, 0.0f}
			},
			{
				.pos = {0.0f, 1.0f, 0.0f},
				.color = {1.0f, 1.0f, 1.0f, 0.0f}
			}
		};
		sync = modelCreateFlat("sync", 3, syncMesh);
	}

	auto identity = mat4(1.0f);
	modelDraw(sync, 1, nullptr, nullptr, &identity);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, milliseconds(100));
}

/**
 * Resize the rendering viewport, preferably to window size. Recreates the
 * matrices and framebuffers as needed.
 * @param size New viewport size in pixels
 */
static void rendererResize(uvec2 size)
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
	renderFbColor.create("renderFbColor", window->size);
	renderFbDepthStencil.create("renderFbDepthStencil", window->size);

	// Set up matrices and framebuffer textures
	rendererResize(window->size);

	// Put framebuffers together
	renderFb.attach(renderFbColor, Attachment::Color0);
	renderFb.attach(renderFbDepthStencil, Attachment::DepthStencil);

	renderFb.bind();

	// Create built-in shaders
	blit.create("blit", BlitVertSrc, BlitFragSrc);
	delinearize.create("delinearize", DelinearizeVertSrc, DelinearizeFragSrc);

	L.debug("Created renderer for window \"%s\"", window->title);
}

void rendererCleanup(void)
{
	if (!initialized) return;
	modelDestroy(sync);
	sync = nullptr;
	delinearize.destroy();
	blit.destroy();
	renderFbDepthStencil.destroy();
	renderFbColor.destroy();
	renderFb.destroy();
	window->deactivateContext();
	L.debug("Destroyed renderer for window \"%s\"", window->title);
	initialized = false;
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
	delinearize.bind();
	delinearize.image = renderFbColor;
	glDrawArrays(GL_TRIANGLES, 0, 3);
	window->flip();
	if (syncEnabled)
		rendererSync();
	glEnable(GL_BLEND);
}

void rendererBlit(Texture<PixelFmt::RGBA_u8>& t, GLfloat boost)
{
	blit.bind();
	blit.image = t;
	blit.boost = boost;
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void rendererBlit(Texture<PixelFmt::RGBA_f16>& t, GLfloat boost)
{
	blit.bind();
	blit.image = t;
	blit.boost = boost;
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
