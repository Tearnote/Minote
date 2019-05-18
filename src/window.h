#ifndef WINDOW_H
#define WINDOW_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

extern GLFWwindow* window;

void initWindow(void);
void cleanupWindow(void);

#endif