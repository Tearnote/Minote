/**
 * Implementation of game.hpp
 * @file
 */

#include "game.hpp"

#include "engine/engine.hpp"
#include "engine/mapper.hpp"
#include "engine/model.hpp"
#include "engine/frame.hpp"
#include "store/shaders.hpp"
#include "particles.hpp"
#include "bloom.hpp"
#include "debug.hpp"
#include "play.hpp"
#include "font.hpp"
#include "text.hpp"

namespace minote {

#ifndef NDEBUG
/**
 * Error callback forwarding OpenGL debug context errors to the logging system.
 * @param source
 * @param type
 * @param severity
 * @param message
 * @see https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
 */
static void APIENTRY debugMessageCallback(GLenum const source,
	GLenum const type, GLuint, GLenum const severity, GLsizei,
	GLchar const* const message, void const*)
{
	char const* const sourceStr = [=] {
		switch (source) {
		case GL_DEBUG_SOURCE_API:
			return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			return "Window";
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			return "Shader compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			return "3rd party";
		case GL_DEBUG_SOURCE_APPLICATION:
			return "Application";
		case GL_DEBUG_SOURCE_OTHER:
			return "Other";
		default:
			return "Unknown";
		}
	}();
	char const* const typeStr = [=] {
		switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			return "deprecated behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			return "undefined behavior";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "portability concern";
		case GL_DEBUG_TYPE_PERFORMANCE:
			return "performance concern";
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
	auto const logFunc = [=] {
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

	std::invoke(logFunc, L, "[OpenGL][%s] %s: %s", sourceStr, typeStr, message);
}
#endif //NDEBUG

/**
 * Temporary replacement for a settings menu.
 */
static void gameDebug(Frame& frame, bool& sync)
{
	if (nk_begin(nkCtx(), "Settings", nk_rect(1070, 30, 180, 220),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE
			| NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(nkCtx(), 20, 1);
		sync = nk_check_label(nkCtx(), "GPU synchronization", sync);
		nk_label(nkCtx(), "Antialiasing:", NK_TEXT_LEFT);
		if (nk_option_label(nkCtx(), "None", frame.aa == Samples::_1))
			frame.changeAA(Samples::_1);
		if (nk_option_label(nkCtx(), "MSAA 2x", frame.aa == Samples::_2))
			frame.changeAA(Samples::_2);
		if (nk_option_label(nkCtx(), "MSAA 4x", frame.aa == Samples::_4))
			frame.changeAA(Samples::_4);
		if (nk_option_label(nkCtx(), "MSAA 8x", frame.aa == Samples::_8))
			frame.changeAA(Samples::_8);
	}
	nk_end(nkCtx());
}

void game(Window& window)
{
	// OpenGL setup
	window.activateContext();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		L.fail("Failed to initialize OpenGL");
	glfwSwapInterval(1); // Enable vsync
#ifndef NDEBUG // Enable debug context features
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Our log handling is fast
	glDebugMessageCallback(debugMessageCallback, nullptr);
#endif //NDEBUG

	// Initialization
	Mapper mapper;

	Shaders shaders;
	shaders.create();
	defer { shaders.destroy(); };

	Frame frame;
	frame.create(window.size, shaders);
	defer { frame.destroy(); };

	Scene scene;

	Models models;
	models.create(shaders);
	defer { models.destroy(); };

	Engine engine = {
		.window = window,
		.mapper = mapper,
		.frame = frame,
		.scene = scene,
		.shaders = shaders,
		.models = models
	};

	fontInit();
	defer { fontCleanup(); };
	textInit();
	defer { textCleanup(); };
	bloomInit(window);
	defer { bloomCleanup(); };
#ifdef MINOTE_DEBUG
	debugInit();
	defer { debugCleanup(); };
#endif //MINOTE_DEBUG
	playInit();
	defer { playCleanup(); };
	particlesInit();

	Draw<> clear = {
		.clearColor = true,
		.clearDepthStencil = true,
		.clearParams = {
			.color = {0.0185f, 0.029f, 0.0944f, 1.0f}
		}
	};
	engine.scene.background = engine.scene.ambientLight = {0.0185f, 0.029f, 0.0944f};

	bool hardSync = true;
	DrawParams syncParams = {
		.culling = false,
		.depthTesting = false
	};

	// Main loop
	while (!window.isClosing()) {
		// Update state
		mapper.mapKeyInputs(window);
#ifdef MINOTE_DEBUG
		debugUpdate();
		gameDebug(frame, hardSync);
#endif //MINOTE_DEBUG
		playUpdate(window, mapper);
		particlesUpdate();

		// Draw frame
		frame.begin(window.size);
		scene.updateMatrices(frame.size);
		clear.framebuffer = frame.fb;
		clear.draw();
		playDraw(engine);
		particlesDraw(engine);
		frame.resolveAA();
		textQueue(FontJost, 3.0f, {6.05, 1.95, 0}, {1.0f, 1.0f, 1.0f, 0.25f}, "Text test.");
		textQueue(FontJost, 3.0f, {6, 2, 0}, {0.0f, 0.0f, 0.0f, 1.0f}, "Text test");
		textDraw(engine);
		bloomApply(engine);
#ifdef MINOTE_DEBUG
		debugDraw(engine);
#endif //MINOTE_DEBUG
		frame.end();
		window.flip();
		if (hardSync) {
			syncParams.viewport.size = frame.size;
			models.sync.draw(*frame.fb, scene, syncParams);
		}
	}
}

}
