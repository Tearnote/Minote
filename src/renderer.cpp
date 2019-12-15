// Minote - renderer.cpp

#include "renderer.h"

#include "glad/glad.h"
#include "log.h"

Renderer::Renderer(Window& w)
:window{w}
{
	window.attachContext();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error{"Failed to initialize OpenGL"};
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB); // Switch to Linear RGB
	glEnable(GL_MULTISAMPLE); // Enable MSAA

	Log::info("OpenGL renderer initialized");
}

auto Renderer::render() -> void
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	window.swapBuffers();
}

Renderer::~Renderer()
{
	window.detachContext();
}
