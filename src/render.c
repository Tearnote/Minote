#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "log.h"
#include "window.h"
#include "thread.h"
#include "state.h"

thread rendererThreadID = 0;

static void renderFrame() {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void initRenderer() {
	glfwMakeContextCurrent(window);
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit("Failed to initialize GLAD");
		cleanupRenderer();
		exit(1);
	}
	glfwSwapInterval(1); // Enable vsync
}

static void cleanupRenderer() {
	glfwMakeContextCurrent(NULL);
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