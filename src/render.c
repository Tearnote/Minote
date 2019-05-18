#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>

#include "log.h"
#include "window.h"
#include "thread.h"
#include "state.h"

thread rendererThreadID = 0;

static void renderFrame(void) {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void cleanupRenderer(void) {
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