// Minote - render.c
// Wild unreadable OpenGL in tall grass

#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "linmath/linmath.h"
#include "log.h"
#include "window.h"
#include "thread.h"
#include "state.h"
#include "minorender.h"
#include "util.h"
#include "gameplay.h"
// Damn that's a lot of includes

#define destroyShader \
        glDeleteShader

thread rendererThreadID = 0;
mat4x4 camera = {};
mat4x4 projection = {};

int viewportWidth = DEFAULT_WIDTH; //SYNC viewportMutex
int viewportHeight = DEFAULT_HEIGHT; //SYNC viewportMutex
float viewportScale = 0.0f; //SYNC viewportMutex
bool viewportDirty = true; //SYNC viewportMutex
mutex viewportMutex = newMutex;

// Thread-local copy of the game state being rendered
static struct game *gameSnap = NULL;

// Compiles a shader from source
static GLuint createShader(const GLchar *source, GLenum type)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		logError("Failed to compile shader: %s", infoLog);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint createProgram(const GLchar *vertexShaderSrc,
                     const GLchar *fragmentShaderSrc)
{
	GLuint vertexShader =
		createShader(vertexShaderSrc, GL_VERTEX_SHADER);
	if (vertexShader == 0)
		return 0;
	GLuint fragmentShader =
		createShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);
	if (fragmentShader == 0) { // Proper cleanup, how fancy
		destroyShader(vertexShader);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	GLint linkStatus = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		logError("Failed to link program: %s", infoLog);
		glDeleteProgram(program);
		program = 0;
	}
	destroyShader(fragmentShader);
	destroyShader(vertexShader);
	return program;
}

static void renderFrame(void)
{
	lockMutex(&viewportMutex);
	if (viewportDirty) {
		glViewport(0, 0, viewportWidth, viewportHeight);
		mat4x4_perspective(projection, radf(45.0f),
		                   (float)viewportWidth / (float)viewportHeight,
		                   PROJECTION_NEAR, PROJECTION_FAR);
		viewportDirty = false;
	}
	unlockMutex(&viewportMutex);

	// Make a local copy of the game state instead
	// of locking it for the entire duration of rendering
	lockMutex(&gameMutex);
	memcpy(gameSnap, app->game, sizeof(*gameSnap));
	unlockMutex(&gameMutex);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderMino();
}

static void cleanupRenderer(void)
{
	cleanupMinoRenderer();
	if (gameSnap) {
		free(gameSnap);
		gameSnap = NULL;
	}
	// glfwTerminate() hangs if other threads have a current context
	glfwMakeContextCurrent(NULL);
}

static void initRenderer(void)
{
	// Activate the thread for rendering
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit("Failed to initialize OpenGL");
		cleanupRenderer();
		exit(1);
	}
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);

	gameSnap = allocate(sizeof(*gameSnap));
	mat4x4_translate(camera, 0.0f, 0.0f, -10.0f);

	initMinoRenderer();

	logInfo("OpenGL renderer initialized");
}

void *rendererThread(void *param)
{
	(void)param;

	initRenderer();

	while (isRunning()) {
		renderFrame();
		// Fix graphics not updating in windowed mode with Intel graphics
		glFinish();
		// Blocks until next vertical refresh because of vsync, so no sleep is needed
		glfwSwapBuffers(window);
	}

	cleanupRenderer();
	return NULL;
}

void resizeRenderer(int width, int height)
{
	lockMutex(&viewportMutex);
	viewportDirty = true;
	viewportWidth = width;
	viewportHeight = height;
	unlockMutex(&viewportMutex);
}

void rescaleRenderer(float scale)
{
	lockMutex(&viewportMutex);
	viewportDirty = true;
	viewportScale = scale;
	unlockMutex(&viewportMutex);
}
