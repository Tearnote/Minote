#ifndef RENDER_H
#define RENDER_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath.h"
#include "thread.h"

extern thread rendererThreadID;
extern mat4x4 projection;
// Size of the virtual coordinate system
extern int renderWidth;
extern int renderHeight;

void* rendererThread(void* param);
#define spawnRenderer() spawnThread(&rendererThreadID, rendererThread, NULL, "rendererThread")
#define awaitRenderer() awaitThread(rendererThreadID)

void resizeRenderer(int width, int height); // Thread-safe

GLuint createProgram(const GLchar* vertexShaderSrc, const GLchar* fragmentShaderSrc);
#define destroyProgram glDeleteProgram

#endif