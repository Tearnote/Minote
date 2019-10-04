// Minote - render.h
// A thread that periodically presents the game's state on the screen
// Manages the OpenGL context of the game window

#ifndef RENDER_H
#define RENDER_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath/linmath.h"
#include "thread.h"

#define PROJECTION_NEAR 0.1f
#define PROJECTION_FAR 100.0f

// Convert degrees to radians
#define radf(x) \
        ((x) * M_PI / 180.0)

extern mat4x4 camera;
extern mat4x4 projection;
extern vec3 lightPosition; // In view space
extern vec3 tintColor;

extern thread rendererThreadID;

void *rendererThread(void *param);
#define spawnRenderer() \
        spawnThread(&rendererThreadID, rendererThread, NULL, "rendererThread")
#define awaitRenderer() \
        awaitThread(rendererThreadID)

// These two are called by window events

void resizeRenderer(int width, int height); //SYNC safe
void rescaleRenderer(float scale); //SYNC safe

// Compiles and binds a pair of shaders
GLuint createProgram(const GLchar *vertexShaderSrc,
                     const GLchar *fragmentShaderSrc);
#define destroyProgram \
        glDeleteProgram

#endif // RENDER_H
