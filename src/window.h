#ifndef WINDOW_H
#define WINDOW_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

extern GLFWwindow* window;

void initWindow(void);
void cleanupWindow(void);

#endif