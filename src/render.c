// Minote - render.c
// Wild unreadable OpenGL in tall grass

#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
mat4x4 projection = {};
// Calculated from the aspect ratio before the first frame is drawn
int renderWidth = 0;
int renderHeight = 360;
float renderScale = 1.0f;

int viewportWidth = DEFAULT_WIDTH; //SYNC viewportMutex
int viewportHeight = DEFAULT_HEIGHT; //SYNC viewportMutex
float viewportScale = 0.0f; //SYNC viewportMutex
bool viewportDirty = true; //SYNC viewportMutex
mutex viewportMutex = newMutex;

// Thread-local copy of the game state being rendered
static struct gameState *gameSnap = NULL;

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
	// This is true on the first frame, so this section is also used
	// to initialize some values
	if (viewportDirty) {
		viewportDirty = false;
		glViewport(0, 0, viewportWidth, viewportHeight);
		renderWidth = (int)((float)viewportWidth /
		                    (float)viewportHeight *
		                    (float)renderHeight);
		mat4x4_ortho(projection, 0.0f,
		             (float)renderWidth, (float)renderHeight,
		             0.0f, -1.0f, 1.0f);
		renderScale = viewportScale;
	}
	unlockMutex(&viewportMutex);

	// Make a local copy of the game state instead
	// of locking it for the entire duration of rendering
	lockMutex(&gameMutex);
	memcpy(gameSnap, app->game, sizeof(*gameSnap));
	unlockMutex(&gameMutex);

	// Just a random background color
	glClearColor(0.03f, 0.07f, 0.07f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Render minos
	queueMinoPlayfield(gameSnap->playfield);
	queueMinoPlayer(&gameSnap->player);
	queueMinoPreview(&gameSnap->player);
	renderMino();
}

static void cleanupRenderer(void)
{
	cleanupMinoRenderer();
	free(gameSnap);
	// glfwTerminate() hangs if other threads have a current context
	glfwMakeContextCurrent(NULL);
}

static void initRenderer(void)
{
	// Activate the thread for rendering
	glfwMakeContextCurrent(window);
	// Not possible to get an error message out of glad?
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit("Failed to initialize GLAD");
		cleanupRenderer();
		exit(1);
	}
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB); // Enable gamma-correct rendering

	initMinoRenderer();

	gameSnap = allocate(sizeof(*gameSnap));

	logInfo("OpenGL renderer initialized");
}

void *rendererThread(void *param)
{
	(void)param;

	initRenderer();

	while (isRunning()) {
		renderFrame();
		// Fix graphics not updating in windowed mode with Intel graphics
		// Need to test if this lowers performance
		glGetError();
		// Blocks until next vertical refresh because of vsync, so no sleep is needed
		glfwSwapBuffers(window);
	}

	cleanupRenderer();
	return NULL;
}

void resizeRenderer(int width, int height)
{
	lockMutex(&viewportMutex);
	viewportWidth = width;
	viewportHeight = height;
	viewportDirty = true;
	unlockMutex(&viewportMutex);
}

void rescaleRenderer(float scale)
{
	lockMutex(&viewportMutex);
	viewportScale = scale;
	viewportDirty = true;
	unlockMutex(&viewportMutex);
}
