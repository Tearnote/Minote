/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.h"

#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "util.h"

struct Renderer {
	Window* window;
	Log* log;
};

Renderer* rendererCreate(Window* w, Log* l)
{
	assert(w);
	assert(l);
	Renderer* r = alloc(sizeof(*r));
	r->window = w;
	r->log = l;

	windowContextActivate(r->window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit(r->log, "Failed to initialize OpenGL");
		exit(EXIT_FAILURE);
	}
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);
	return r;
}

void rendererDestroy(Renderer* r)
{
	assert(r);
	windowContextDeactivate(r->window);
	free(r);
}

void rendererClear(Renderer* r, Color3 color)
{
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear((uint32_t)GL_COLOR_BUFFER_BIT | (uint32_t)GL_DEPTH_BUFFER_BIT);
}

void rendererFlip(Renderer* r)
{
	windowFlip(r->window);
}
