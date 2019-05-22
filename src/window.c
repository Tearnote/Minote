#include "window.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "main.h"
#include "log.h"
#include "render.h"

GLFWwindow* window = NULL;
int windowWidth = DEFAULT_WIDTH;
int windowHeight = DEFAULT_HEIGHT;

static void windowResizeCallback(GLFWwindow* w, int width, int height) {
	(void)w;
	windowWidth = width;
	windowHeight = height;
	resizeRenderer(width, height);
}

void initWindow(void) {
	if(glfwInit() == GLFW_FALSE) {
		logCritGLFW("Failed to initialize GLFW");
		exit(1);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	window = glfwCreateWindow(windowWidth, windowHeight, APP_NAME, NULL, NULL);
	if(window == NULL) {
		logCritGLFW("Failed to create a window");
		exit(1);
	}
	glfwSetFramebufferSizeCallback(window, windowResizeCallback);
	logInfo("Created a %dx%d window", windowWidth, windowHeight);
}

void cleanupWindow(void) {
	glfwTerminate();
}