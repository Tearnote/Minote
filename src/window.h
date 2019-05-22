#ifndef WINDOW_H
#define WINDOW_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

extern GLFWwindow* window;
extern int windowWidth;
extern int windowHeight;

void initWindow(void);
void cleanupWindow(void);

#endif