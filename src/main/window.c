// Minote - main/window.c

#include "main/window.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "util/log.h"
#include "util/util.h"
#include "global/settings.h"
#include "main/main.h"
#include "render/render.h"

GLFWwindow *window = NULL;
int windowWidth = DEFAULT_WIDTH;
int windowHeight = DEFAULT_HEIGHT;
float windowScale = 1.0f;

// Bubble the geometry change up to the renderer thread
static void framebufferResizeCallback(GLFWwindow *w, int width, int height)
{
	(void)w;

	windowWidth = width;
	windowHeight = height;
	resizeRenderer(width, height);
	logDebug("Framebuffer resized to %dx%d", windowWidth, windowHeight);
}

// Bubble the scaling change up to the renderer thread
// For some reason yScale is equal to 0???
static void windowScaleCallback(GLFWwindow *w, float xScale, float yScale)
{
	(void)w, (void)yScale;

	windowScale = xScale;
	rescaleRenderer(xScale);
	logDebug("DPI scaling changed to %f", windowScale);
}

void initWindow(void)
{
	if (glfwInit() == GLFW_FALSE) {
		logCritGLFW("Failed to initialize GLFW");
		exit(EXIT_FAILURE);
	}
	// Request OpenGL 3.3 core profile context for use by the renderer
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // __APPLE__
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); // DPI aware
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE); // Linear gamma
	glfwWindowHint(GLFW_SAMPLES, 4); // 4xMSAA
	if (getSettingBool(SettingFullscreen)) {
		const GLFWvidmode
			*mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		window = glfwCreateWindow(mode->width, mode->height,
		                          APP_NAME" "APP_VERSION,
		                          glfwGetPrimaryMonitor(), NULL);
	} else {
		window = glfwCreateWindow(windowWidth, windowHeight,
		                          APP_NAME" "APP_VERSION,
		                          NULL, NULL);
	}
	if (window == NULL) {
		logCritGLFW("Failed to create a window");
		exit(EXIT_FAILURE);
	}
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetWindowContentScaleCallback(window, windowScaleCallback);

	// An initial check is required to get correct values for non-100% scaling
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	framebufferResizeCallback(window, windowWidth, windowHeight);
	glfwGetWindowContentScale(window, &windowScale, NULL);
	windowScaleCallback(window, windowScale, 0);

	logInfo("Created a %dx%d *%f window",
	        windowWidth, windowHeight, windowScale);
}

void cleanupWindow(void)
{
	glfwTerminate();
}
