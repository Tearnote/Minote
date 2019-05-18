#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "window.h"
#include "thread.h"
#include "state.h"

thread rendererThreadID = 0;
state* gameSnap = NULL;

static void renderFrame(void) {
	lockMutex(&stateMutex);
	memcpy(gameSnap, game, sizeof(state));
	unlockMutex(&stateMutex);
	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void cleanupRenderer(void) {
	free(gameSnap);
	glfwMakeContextCurrent(NULL); // glfwTerminate() hangs if other threads have a current context
}

static void initRenderer(void) {
	glfwMakeContextCurrent(window);
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { // Not possible to get an error message?
		logCrit("Failed to initialize GLAD");
		cleanupRenderer();
		exit(1);
	}
	glfwSwapInterval(1); // Enable vsync
	
	gameSnap = malloc(sizeof(state));
	
	logInfo("OpenGL renderer initialized");
}

void* rendererThread(void* param) {
	(void)param;
	
	initRenderer();
	
	while(isRunning()) {
		renderFrame();
		glfwSwapBuffers(window);
	}
	
	cleanupRenderer();
	return NULL;
}