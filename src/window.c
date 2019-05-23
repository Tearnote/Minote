#include "window.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "main.h"
#include "log.h"
#include "render.h"

GLFWwindow* window = NULL;
int windowWidth = DEFAULT_WIDTH;
int windowHeight = DEFAULT_HEIGHT;
float windowScale = 1.0f;

static void framebufferResizeCallback(GLFWwindow* w, int width, int height) {
	(void)w;
	windowWidth = width;
	windowHeight = height;
	resizeRenderer(width, height);
	logDebug("Framebuffer resized to %dx%d", windowWidth, windowHeight);
}

static void windowScaleCallback(GLFWwindow* w, float xscale, float yscale) {
	(void)w, (void)yscale;
	windowScale = xscale;
	logDebug("DPI scaling changed to %f", windowScale);
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
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	window = glfwCreateWindow(windowWidth, windowHeight, APP_NAME, NULL, NULL);
	if(window == NULL) {
		logCritGLFW("Failed to create a window");
		exit(1);
	}
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(window, windowScaleCallback);
	
	// One run is required to get correct initial values for non-100% scaling
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	framebufferResizeCallback(window, windowWidth, windowHeight);
	glfwGetWindowContentScale(window, &windowScale, NULL);
	windowScaleCallback(window, windowScale, 0);
	
	logInfo("Created a %dx%d *%f window", windowWidth, windowHeight, windowScale);
}

void cleanupWindow(void) {
	glfwTerminate();
}