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

/// Semantic rename of OpenGL shader object ID
typedef GLuint Shader;

/// Semantic rename of OpenGL shader program object ID
typedef GLuint Program;

/// List of shader programs supported by the renderer
typedef enum ProgramType {
	ProgramNone,
	ProgramFlat, ///< @todo rendererDrawFlat()
//	ProgramPhong,
//	ProgramMsdf,
			ProgramSize
} ProgramType;

/// Struct that encapsulates a ::Shader's source code
typedef struct ShaderSource {
	const char* name; ///< Human-readable name for identification
	const char* source; ///< Full GLSL source code
} ShaderSource;

/// Fragment and vertex ::Shader sources to be linked into a single ::Program
typedef struct ProgramSources {
	ShaderSource vert; ///< Source of the vertex ::Shader
	ShaderSource frag; ///< Source of the fragment ::Shader
} ProgramSources;

///< Constant list of shader program sources
static const ProgramSources ShaderPaths[ProgramSize] = {
		[ProgramFlat] = {
				.vert = (ShaderSource){
						.name = "flat.vert",
						.source = (GLchar[]){
#include "flat.vert"
								, '\0'}},
				.frag = (ShaderSource){
						.name = "flat.frag",
						.source = (GLchar[]){
#include "flat.frag"
								, '\0'}}
		}
};

struct Renderer {
	Window* window; ///< ::Window object the ::Renderer is attached too
	Log* log; ///< ::Log to use for rendering-related messages
	Program programs[ProgramSize];
};

/**
 * Create an OpenGL shader object. The shader is compiled and ready for linking.
 * @param r The ::Renderer object
 * @param source Struct containing shader's name and GLSL source code
 * @param type GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @return Newly created ::Shader's ID
 */
static Shader shaderCreate(Renderer* r, ShaderSource source, GLenum type)
{
	assert(r);
	assert(source.name);
	assert(source.source);
	assert(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);
	Shader shader = glCreateShader(type);
	glShaderSource(shader, 1, &source.source, null);
	glCompileShader(shader);
	GLint compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, null, infoLog);
		logError(r->log, u8"Failed to compile shader %s: %s",
				source.name, infoLog);
		glDeleteShader(shader);
		return 0;
	}
	assert(shader);
	logDebug(r->log, u8"Compiled %s shader %s",
			type == GL_VERTEX_SHADER ? "vertex" : "fragment", source.name);
	return shader;
}

/**
 * Destroy a ::Shader instance. The shader ID becomes invalid and cannot be
 * used again.
 * @param r The ::Renderer object
 * @param shader ::Shader ID to destroy
 */
static void shaderDestroy(Renderer* r, Shader shader)
{
	glDeleteShader(shader);
}

/**
 * Create a ::Program instance by compiling and linking a vertex shader and a
 * fragment shader.
 * @param r The ::Renderer object
 * @param sources Struct containing vertex and fragment shaders' GLSL sources
 * @return Newly created ::Program's ID
 */
static Program programCreate(Renderer* r, ProgramSources sources)
{
	assert(r);
	assert(sources.vert.name);
	assert(sources.vert.source);
	assert(sources.frag.name);
	assert(sources.frag.source);
	Shader vert = shaderCreate(r, sources.vert, GL_VERTEX_SHADER);
	if (vert == 0)
		return 0;
	Shader frag = shaderCreate(r, sources.frag, GL_FRAGMENT_SHADER);
	if (frag == 0) {
		shaderDestroy(r, vert); // Proper cleanup, how fancy
		return 0;
	}

	Program program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	GLint linkStatus = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetProgramInfoLog(program, 512, null, infoLog);
		logError(r->log, u8"Failed to link shader program %s+%s: %s",
				sources.vert.name, sources.frag.name, infoLog);
		glDeleteProgram(program);
		program = 0;
	}
	shaderDestroy(r, frag);
	frag = 0;
	shaderDestroy(r, vert);
	vert = 0;
	logDebug(r->log, u8"Linked shader program %s+%s",
			sources.vert.name, sources.frag.name);
	return program;
}

/**
 * Destroy a ::Program instance. The program ID becomes invalid and cannot be
 * used again.
 * @param r The ::Renderer object
 * @param shader ::Program ID to destroy
 */
static void programDestroy(Renderer* r, Program program)
{
	glDeleteProgram(program);
}

Renderer* rendererCreate(Window* window, Log* log)
{
	assert(window);
	assert(log);
	Renderer* r = alloc(sizeof(*r));
	r->window = window;
	r->log = log;

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

	for (size_t i = ProgramNone + 1; i < ProgramSize; i += 1)
		r->programs[i] = programCreate(r, ShaderPaths[i]);

	logDebug(r->log, "Created renderer for window \"%s\"",
			windowGetTitle(r->window));
	return r;
}

void rendererDestroy(Renderer* r)
{
	assert(r);
	for (size_t i = ProgramNone + 1; i < ProgramSize; i += 1) {
		programDestroy(r, r->programs[i]);
		r->programs[i] = 0;
	}
	windowContextDeactivate(r->window);
	logDebug(r->log, "Destroyed renderer for window \"%s\"",
			windowGetTitle(r->window));
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
