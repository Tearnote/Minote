#include "window.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "main.h"
#include "log.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

GLFWwindow* window = NULL;

static void framebufferSizeCallback(GLFWwindow* w, int width, int height) {
	(void)w;
	glViewport(0, 0, width, height);
}

void initWindow() {
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
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, APP_NAME, NULL, NULL);
	if(window == NULL) {
		logCritGLFW("Failed to create a window");
		exit(1);
	}
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	logInfo("Created a %dx%d window", SCREEN_WIDTH, SCREEN_HEIGHT);
}

void cleanupWindow() {
	glfwTerminate();
}