// Minote - render.h
// A thread that periodically presents the game's state on the screen
// Manages the OpenGL context of the game window
// The coordinate system is pixel-based for convenience, but is not guaranteed to match up with screen pixels

#ifndef RENDER_H
#define RENDER_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath/linmath.h"
#include "thread.h"

extern thread rendererThreadID;
extern mat4x4 projection;

// Geometry of the virtual coordinate system

extern int renderWidth;
extern int renderHeight;
extern float renderScale; // Not needed for computing position, but important for UI scaling

void* rendererThread(void* param);
#define spawnRenderer() spawnThread(&rendererThreadID, rendererThread, NULL, "rendererThread")
#define awaitRenderer() awaitThread(rendererThreadID)

// These two are called by window events

void resizeRenderer(int width, int height); //SYNC safe
void rescaleRenderer(float scale); //SYNC safe

// Compiles and binds a pair of shaders
GLuint createProgram(const GLchar* vertexShaderSrc, const GLchar* fragmentShaderSrc);
#define destroyProgram glDeleteProgram

#endif