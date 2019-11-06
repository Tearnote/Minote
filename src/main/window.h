// Minote - window.h
// Handles window creation and geometry
// Renderers should use render.h for viewport geometry:
// Window size can be different from viewport size,
// and window variables are handled by a different thread than rendering

#ifndef WINDOW_H
#define WINDOW_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

extern GLFWwindow *window;
extern int windowWidth;
extern int windowHeight;
extern float windowScale;

void initWindow(void);
void cleanupWindow(void);

#endif // WINDOW_H
