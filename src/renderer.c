/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.h"

#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath/linmath.h"
#include "util.h"

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

/// Semantic rename of OpenGL shader object ID
typedef GLuint Shader;

typedef struct Program {
	GLuint id;
	//GLint cameraAttr;
	GLint projectionAttr;
} Program;

/// Struct that encapsulates a ::Shader's source code
typedef struct ShaderSource {
	const char* name; ///< Human-readable name for identification
	const GLchar* source; ///< Full GLSL source code
} ShaderSource;

/// Fragment and vertex ::Shader sources to be linked into a single ::Program
typedef struct ProgramSources {
	ShaderSource vert; ///< Source of the vertex ::Shader
	ShaderSource frag; ///< Source of the fragment ::Shader
} ProgramSources;

///< Constant list of shader program sources
static const ProgramSources ShaderPaths[ProgramSize] = {
		[ProgramFlat] = {
				.vert = {
						.name = "flat.vert",
						.source = (GLchar[]){
#include "flat.vert"
								, '\0'}},
				.frag = {
						.name = "flat.frag",
						.source = (GLchar[]){
#include "flat.frag"
								, '\0'}}
		}
};

struct Renderer {
	Log* log; ///< ::Log to use for rendering-related messages
	Window* window; ///< ::Window object the ::Renderer is attached too
	Size2i size; ///< Size of the rendering viewport in pixels
	mat4x4 projection; ///< Projection matrix (perspective transform)
	Program programs[ProgramSize]; ///< Array of usable shader programs
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
		return (Program){0};
	Shader frag = shaderCreate(r, sources.frag, GL_FRAGMENT_SHADER);
	if (frag == 0) {
		shaderDestroy(r, vert); // Proper cleanup, how fancy
		return (Program){0};
	}

	Program program = {0};
	program.id = glCreateProgram();
	glAttachShader(program.id, vert);
	glAttachShader(program.id, frag);
	glLinkProgram(program.id);
	GLint linkStatus = 0;
	glGetProgramiv(program.id, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetProgramInfoLog(program.id, 512, null, infoLog);
		logError(r->log, u8"Failed to link shader program %s+%s: %s",
				sources.vert.name, sources.frag.name, infoLog);
		glDeleteProgram(program.id);
		program.id = 0;
	}
	shaderDestroy(r, frag);
	frag = 0;
	shaderDestroy(r, vert);
	vert = 0;
	/*program.cameraAttr = glGetUniformLocation(program.id, "camera");
	assert(program.cameraAttr != -1);*/
	program.projectionAttr = glGetUniformLocation(program.id, "projection");
	assert(program.projectionAttr != -1);
	logDebug(r->log, u8"Linked shader program %s+%s",
			sources.vert.name, sources.frag.name);
	return program;
}

/**
 * Destroy a ::Program instance. The program ID becomes invalid and cannot be
 * used again.
 * @param r The ::Renderer object
 * @param program ::Program ID to destroy
 */
static void programDestroy(Renderer* r, Program program)
{
	glDeleteProgram(program.id);
	program.id = 0;
}

/**
 * Resize the ::Renderer viewport, preferably to ::Window size. Recreates the
 * matrices as needed.
 * @param r The ::Renderer object
 * @param size New viewport size in pixels
 */
static void rendererResize(Renderer* r, Size2i size)
{
	assert(r);
	assert(size.x);
	assert(size.y);
	r->size.x = size.x;
	r->size.y = size.y;
	glViewport(0, 0, size.x, size.y);
	mat4x4_perspective(r->projection, radf(45.0f),
			(float)size.x / (float)size.y, ProjectionNear, ProjectionFar);
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

	rendererResize(r, windowGetSize(r->window));

	logDebug(r->log, "Created renderer for window \"%s\"",
			windowGetTitle(r->window));
	return r;
}

void rendererDestroy(Renderer* r)
{
	assert(r);
	for (size_t i = ProgramNone + 1; i < ProgramSize; i += 1) {
		programDestroy(r, r->programs[i]);
		r->programs[i] = (Program){0};
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
	Size2i windowSize = windowGetSize(r->window);
	if (r->size.x != windowSize.x || r->size.y != windowSize.y)
		rendererResize(r, windowSize);
}

////////////////////////////////////////////////////////////////////////////////

/// Semantic rename of OpenGL vertex buffer object ID
typedef GLuint VertexBuffer;

/// Semantic rename of OpenGL vertex array object ID
typedef GLuint VertexArray;

struct Model {
	ProgramType type;
	const char* name;
	VertexBuffer triangles;
	size_t numTriangles;
	VertexBuffer tints;
	VertexBuffer transforms;
	VertexArray vao;
};

Model* modelCreate(Renderer* r, ProgramType type, const char* name,
		size_t numTriangles, Triangle triangles[numTriangles])
{
	assert(type > ProgramNone && type < ProgramSize);
	assert(numTriangles);
	assert(triangles);
	Model* m = alloc(sizeof(*m));
	m->type = type;
	m->name = name;
	glGenBuffers(1, &m->triangles);
	glBindBuffer(GL_ARRAY_BUFFER, m->triangles);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle) * numTriangles, triangles,
			GL_STATIC_DRAW);
	m->numTriangles = numTriangles;
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->transforms);
	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			(void*)offsetof(Vertex, color));
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Color4),
			(void*)0);
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vec4),
			(void*)0);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(vec4),
			(void*)sizeof(vec4));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4),
			(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4),
			(void*)(sizeof(vec4) * 3));
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	logDebug(r->log, u8"Model %s created", m->name);
	return m;
}

void modelDestroy(Renderer* r, Model* m)
{
	glDeleteVertexArrays(1, &m->vao);
	glDeleteBuffers(1, &m->transforms);
	glDeleteBuffers(1, &m->tints);
	glDeleteBuffers(1, &m->triangles);
	logDebug(r->log, u8"Model %s destroyed", m->name);
	free(m);
	m = null;
}

void modelDraw(Renderer* r, Model* m, size_t instances,
		Color4 tints[instances], mat4x4 transforms[instances])
{
	assert(r);
	assert(m);
	assert(m->vao);
	assert(m->name);
	assert(m->triangles);
	assert(m->tints);
	assert(m->transforms);
	assert(tints);
	assert(transforms);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Color4) * instances, tints,
			GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, transforms,
			GL_STREAM_DRAW);
	glUseProgram(r->programs[m->type].id);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(0, 1, GL_FALSE, r->projection[0]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numTriangles * 3, instances);
}
