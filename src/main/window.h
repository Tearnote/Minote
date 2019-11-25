// Minote - main/window.h
// Handles window creation and geometry

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

extern GLFWwindow *window;

void initWindow(void);
void cleanupWindow(void);

#endif //MAIN_WINDOW_H
