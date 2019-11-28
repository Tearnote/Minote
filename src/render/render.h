// Minote - render/render.h
// A thread that periodically presents the game's state on the screen
// Manages the OpenGL context of the game window

#ifndef RENDER_RENDER_H
#define RENDER_RENDER_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath/linmath.h"

#define PROJECTION_NEAR 0.1f
#define PROJECTION_FAR 100.0f

// Convert degrees to radians
#define radf(x) \
        ((x) * M_TAU / 360.0)

extern mat4x4 camera;
extern mat4x4 projection;
extern vec3 lightPosition; // In view space
extern vec3 tintColor;

// These two are called by window events

void resizeRenderer(int width, int height);
void rescaleRenderer(float scale);

// Compiles and binds a pair of shaders
GLuint createProgram(const GLchar *vertexShaderSrc,
                     const GLchar *fragmentShaderSrc);
#define destroyProgram \
        glDeleteProgram

void spawnRenderer(void);
void awaitRenderer(void);

#endif // RENDER_RENDER_H
