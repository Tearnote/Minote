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
#include "postrender.h"
#include "util.h"
#include "replay.h"
#include "timer.h"
#include "ease.h"
#include "settings.h"
#include "effects.h"
#include "particlerender.h"
// Damn that's a lot of includes

#define destroyShader \
        glDeleteShader

#define BGFADE_LENGTH (1 * SEC)

thread rendererThreadID = 0;
mat4x4 camera = {};
mat4x4 projection = {};
vec3 lightPosition = {};
vec4 lightPositionWorld = {};

int viewportWidth = DEFAULT_WIDTH; //SYNC viewportMutex
int viewportHeight = DEFAULT_HEIGHT; //SYNC viewportMutex
float viewportScale = 1.0f; //SYNC viewportMutex
bool viewportDirty = true; //SYNC viewportMutex
mutex viewportMutex = newMutex;

// Thread-local copy of the game state being rendered
static struct app *snap = NULL;

int lastFrame = -1;
static nsec lastRenderTime = 0;
static nsec timeElapsed = 0;

struct background {
	int level;
	float color[3];
};

static struct background backgrounds[] = {
	{ .level = -1, .color = { 0.262f, 0.533f, 0.849f }}, // Default
	{ .level = 0, .color = { 0.010f, 0.276f, 0.685f }},
	{ .level = 100, .color = { 0.070f, 0.280f, 0.201f }},
	{ .level = 200, .color = { 0.502f, 0.260f, 0.394f }},
	{ .level = 300, .color = { 0.405f, 0.468f, 0.509f }},
	{ .level = 400, .color = { 0.394f, 0.318f, 0.207f }},
	{ .level = 500, .color = { 0.368f, 0.290f, 0.084f }},
	{ .level = 600, .color = { 0.030f, 0.238f, 0.151f }},
	{ .level = 700, .color = { 0.093f, 0.137f, 0.057f }},
	{ .level = 800, .color = { 0.468f, 0.348f, 0.153f }},
	{ .level = 900, .color = { 0.366f, 0.265f, 0.590f }}
};

static int currentBackground = 0;
vec3 tintColor = {};

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

static void updateEffects(void)
{
	struct effect *e;
	while ((e = dequeueEffect())) {
		switch (e->type) {
		case EffectLockFlash:
			triggerLockFlash(e->data);
			free(e->data);
			break;
		case EffectLineClear:
			triggerLineClear(e->data);
			free(e->data);
			break;
		default:
			break;
		}
		free(e);
	}
}

static void updateBackground(void)
{
	int newBackground = 0;

	if (getState(PhaseGameplay) != StateIntro) {
		for (int i = 1; i < countof(backgrounds); i++) {
			if (backgrounds[i].level > snap->game->level)
				break;
			newBackground = i;
		}
	}

	if (currentBackground != newBackground) {
		for (int i = 0; i < 3; i++) {
			addEase(&tintColor[i], tintColor[i],
			        backgrounds[newBackground].color[i],
			        BGFADE_LENGTH / snap->replay->speed,
			        EaseInOutCubic);
		}
		currentBackground = newBackground;
	}
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

static void updateFrame(void)
{
	nsec currentTime = getTime();
	timeElapsed = currentTime - lastRenderTime;
	lastRenderTime = currentTime;

	lockMutex(&viewportMutex);
	if (viewportDirty) {
		glViewport(0, 0, viewportWidth, viewportHeight);
		mat4x4_perspective(projection, radf(45.0f),
		                   (float)viewportWidth / (float)viewportHeight,
		                   PROJECTION_NEAR, PROJECTION_FAR);
		resizePostRender(viewportWidth, viewportHeight);
		viewportDirty = false;
	}
	unlockMutex(&viewportMutex);

	// Make a local copy of the game state
	lockMutex(&appMutex);
	if (app->game) {
		memcpy(snap->game, app->game, sizeof(*snap->game));
	} else { // Gameplay might not be done initializing
		unlockMutex(&appMutex);
		return;
	}
	if (app->replay) {
		memcpy(snap->replay, app->replay, sizeof(*snap->replay));
	} else {
		unlockMutex(&appMutex);
		return;
	}
	unlockMutex(&appMutex);

	vec3 eye = { 0.0f, 12.0f, 32.0f };
	vec3 center = { 0.0f, 12.0f, 0.0f };
	vec3 up = { 0.0f, 1.0f, 0.0f };
	//TODO Manipulate the values to look around
	mat4x4_look_at(camera, eye, center, up);

	vec4 lightPositionTemp;
	mat4x4_mul_vec4(lightPositionTemp, camera, lightPositionWorld);
	copyArray(lightPosition, lightPositionTemp);

	updateEffects();
	updateBackground();
	updateScene(snap->game->combo);
	updateEase();
	updateParticles(); // Needs to be after updateEase

}

static void renderFrame(void)
{
	renderPostStart();

	glClearColor(tintColor[0], tintColor[1], tintColor[2],
	             1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderScene();
	queueMinoPlayfield(snap->game->playfield);
	queueMinoPlayer(&snap->game->player);
	queueMinoGhost(&snap->game->player);
	queueMinoPreview(&snap->game->player);
	renderMino();
	queueBorder(snap->game->playfield);
	renderBorder();
	queueGameplayText(snap->game);
	if (snap->replay->state == ReplayViewing)
		queueReplayText(snap->replay);
	renderText();
	renderParticles();

	renderPostEnd();
}

static void cleanupRenderer(void)
{
	cleanupParticleRenderer();
	cleanupPostRenderer();
	cleanupTextRenderer();
	cleanupBorderRenderer();
	cleanupMinoRenderer();
	cleanupSceneRenderer();
	cleanupEase();
	if (snap->replay)
		free(snap->replay);
	if (snap->game)
		free(snap->game);
	if (snap)
		free(snap);
	snap = NULL;
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

	snap = allocate(sizeof(*snap));
	snap->game = allocate(sizeof(*snap->game));
	snap->replay = allocate(sizeof(*snap->replay));

	mat4x4_translate(camera, 0.0f, -12.0f, -32.0f);
	lightPositionWorld[0] = -8.0f;
	lightPositionWorld[1] = 32.0f;
	lightPositionWorld[2] = 16.0f;
	lightPositionWorld[3] = 1.0f;

	currentBackground = 0;
	copyArray(tintColor, backgrounds[currentBackground].color);

	initEase();
	initSceneRenderer();
	initMinoRenderer();
	initBorderRenderer();
	initTextRenderer();
	initPostRenderer();
	initParticleRenderer();

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
		updateFrame();
		renderFrame();
		// Blocks until next vertical refresh
		glfwSwapBuffers(window);
		// Mitigate GPU buffering
		if (!getSettingBool(SettingNoSync))
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
