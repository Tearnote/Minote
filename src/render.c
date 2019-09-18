// Minote - render.c
// Wild unreadable OpenGL in tall grass

#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "linmath/linmath.h"
#include "log.h"
#include "window.h"
#include "thread.h"
#include "state.h"
#include "scenerender.h"
#include "minorender.h"
#include "borderrender.h"
#include "textrender.h"
#include "util.h"
#include "gameplay.h"
#include "replay.h"
#include "timer.h"
// Damn that's a lot of includes

#define FENCE_COUNT 3

#define destroyShader \
        glDeleteShader

thread rendererThreadID = 0;
mat4x4 camera = {};
mat4x4 projection = {};
vec3 lightPosition = {};

vec4 lightPositionWorld = {};

int viewportWidth = DEFAULT_WIDTH; //SYNC viewportMutex
int viewportHeight = DEFAULT_HEIGHT; //SYNC viewportMutex
float viewportScale = 0.0f; //SYNC viewportMutex
bool viewportDirty = true; //SYNC viewportMutex
mutex viewportMutex = newMutex;

// Thread-local copy of the game state being rendered
static struct game *gameSnap = NULL;
static struct replay *replaySnap = NULL;

static nsec lastRenderTime = 0;
static nsec timeElapsed = 0;

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
	lockMutex(&appMutex);
	if (app->game) {
		memcpy(gameSnap, app->game, sizeof(*gameSnap));
		unlockMutex(&appMutex);
	} else { // Gameplay might not be done initializing
		unlockMutex(&appMutex);
		return;
	}
	if (false) {
		if (app->replay) {
			memcpy(replaySnap, app->replay, sizeof(*replaySnap));
		} else {
			return;
		}
	}

	vec3 eye = { 0.0f, 12.0f, 32.0f };
	vec3 center = { 0.0f, 12.0f, 0.0f };
	vec3 up = { 0.0f, 1.0f, 0.0f };
	//TODO Manipulate the values to look around
	mat4x4_look_at(camera, eye, center, up);

	vec4 lightPositionTemp;
	mat4x4_mul_vec4(lightPositionTemp, camera, lightPositionWorld);
	lightPosition[0] = lightPositionTemp[0];
	lightPosition[1] = lightPositionTemp[1];
	lightPosition[2] = lightPositionTemp[2];

	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearColor(powf(0.48f, 2.2f),
	             powf(0.75f, 2.2f),
	             powf(0.83f, 2.2f),
	             1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderScene();
	queueMinoPlayfield(gameSnap->playfield);
	queueMinoPlayer(&gameSnap->player);
	queueMinoPreview(&gameSnap->player);
	renderMino();
	queueBorder(gameSnap->playfield);
	renderBorder();
	queueGameplayText(gameSnap);
	if (false)
		queueReplayText(replaySnap);
	renderText();
}

static void cleanupRenderer(void)
{
	cleanupTextRenderer();
	cleanupBorderRenderer();
	cleanupMinoRenderer();
	cleanupSceneRenderer();
	if (replaySnap) {
		free(replaySnap);
		replaySnap = NULL;
	}
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
		exit(EXIT_FAILURE);
	}
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);

	gameSnap = allocate(sizeof(*gameSnap));
	replaySnap = allocate(sizeof(*replaySnap));

	mat4x4_translate(camera, 0.0f, -12.0f, -32.0f);
	lightPositionWorld[0] = -8.0f;
	lightPositionWorld[1] = 32.0f;
	lightPositionWorld[2] = 16.0f;
	lightPositionWorld[3] = 1.0f;

	initSceneRenderer();
	initMinoRenderer();
	initBorderRenderer();
	initTextRenderer();

	lastRenderTime = getTime();

	logInfo("OpenGL renderer initialized");
}

// https://danluu.com/latency-mitigation/
static void syncRenderer(void)
{
	queueMinoSync();
	renderMino();
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, MSEC * 100);
}

void *rendererThread(void *param)
{
	(void)param;

	initRenderer();

	while (isRunning()) {
		nsec currentTime = getTime();
		timeElapsed = currentTime - lastRenderTime;
		lastRenderTime = currentTime;
		renderFrame();
		// Blocks until next vertical refresh
		glfwSwapBuffers(window);
		// Mitigate GPU buffering
		syncRenderer();
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
