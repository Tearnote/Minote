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
#include "time.h"

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

/// Semantic rename of OpenGL shader object ID
typedef GLuint Shader;

/// Semantic rename of OpenGL shader program object ID
typedef GLuint Program;

/// Semantic rename of OpenGL uniform location
typedef GLint Uniform;

/// List of model types supported by the renderer
typedef enum ProgramType {
	ProgramTypeNone, ///< zero value
	ProgramTypeFlat, ///< Flat shading and no lighting. Most basic shader
//	ProgramTypePhong, ///< Phong shading. Simple but makes use of light sources
			ProgramTypeSize ///< terminator
} ProgramType;

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

/// Constant list of shader program sources
static const ProgramSources DefaultShaders[ProgramTypeSize] = {
		[ProgramTypeFlat] = {
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

/// Instance of a #ProgramTypeFlat shader program
typedef struct ProgramFlat {
	Program id; ///< ID of the program object
	Uniform camera; ///< Location of "camera" uniform
	Uniform projection; ///< Location of "projection" uniform
	const ProgramSources* sources; ///< Pointer to shader sources for reference
} ProgramFlat;

struct Renderer {
	Log* log; ///< ::Log to use for rendering-related messages
	Window* window; ///< ::Window object the ::Renderer is attached too
	Size2i size; ///< Size of the rendering viewport in pixels
	mat4x4 projection; ///< Projection matrix (perspective transform)
	mat4x4 camera; ///< Camera matrix (world transform)
	ModelFlat* sync; ///< Invisible model used to prevent frame buffering
	ProgramFlat flat; ///< Data of the built-in #ProgramTypeFlat shader
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

	Program program = -1;
	program = glCreateProgram();
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
 * @param program ::Program ID to destroy
 */
static void programDestroy(Renderer* r, Program program)
{
	glDeleteProgram(program);
	program = -1;
}

/**
 * Prevent the driver from buffering commands. Call this after windowFlip()
 * to minimize video latency.
 * @param r The ::Renderer object
 * @see https://danluu.com/latency-mitigation/
 */
static void rendererSync(Renderer* r)
{
	mat4x4 identity = {0};
	mat4x4_identity(identity);
	modelDrawFlat(r, r->sync, 1, (Color4[]){1.0f, 1.0f, 1.0f, 1.0f}, &identity);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, secToNsec(0.1));
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

	// Pick up the OpenGL context
	windowContextActivate(r->window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit(r->log, "Failed to initialize OpenGL");
		exit(EXIT_FAILURE);
	}

	// Set up global OpenGL state
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);

	// Create built-in shaders
	r->flat.id = programCreate(r, DefaultShaders[ProgramTypeFlat]);
	r->flat.sources = &DefaultShaders[ProgramTypeFlat];
	r->flat.projection = glGetUniformLocation(r->flat.id, "projection");
	if (r->flat.projection == -1)
		logWarn(r->log,
				u8"\"projection\" uniform not available in shader program %s+%s",
				r->flat.sources->vert.name, r->flat.sources->frag.name);
	r->flat.camera = glGetUniformLocation(r->flat.id, "camera");
	if (r->flat.camera == -1)
		logWarn(r->log,
				u8"\"camera\" uniform not available in shader program %s+%s",
				r->flat.sources->vert.name, r->flat.sources->frag.name);

	// Set up matrices
	rendererResize(r, windowGetSize(r->window));

	// Set up the camera
	vec3 eye = {0.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(r->camera, eye, center, up);

	// Create sync model
	r->sync = modelCreateFlat(r, "sync", 3, (VertexFlat[]){
			{
					.pos = {0.0f, 0.0f, 0.0f},
					.color = {1.0f, 1.0f, 1.0f, 0.0f}
			},
			{
					.pos = {1.0f, 0.0f, 0.0f},
					.color = {1.0f, 1.0f, 1.0f, 0.0f}
			},
			{
					.pos = {0.0f, 1.0f, 0.0f},
					.color = {1.0f, 1.0f, 1.0f, 0.0f}
			}
	});

	logDebug(r->log, "Created renderer for window \"%s\"",
			windowGetTitle(r->window));
	return r;
}

void rendererDestroy(Renderer* r)
{
	assert(r);
	modelDestroyFlat(r, r->sync);
	r->sync = 0;
	programDestroy(r, r->flat.id);
	r->flat.id = -1;
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
	rendererSync(r);

	Size2i windowSize = windowGetSize(r->window);
	if (r->size.x != windowSize.x || r->size.y != windowSize.y)
		rendererResize(r, windowSize);
}

////////////////////////////////////////////////////////////////////////////////

/// Semantic rename of OpenGL vertex buffer object ID
typedef GLuint VertexBuffer;

/// Semantic rename of OpenGL vertex array object ID
typedef GLuint VertexArray;

struct ModelFlat {
	const char* name;
	VertexBuffer vertices;
	size_t numVertices;
	VertexBuffer tints;
	VertexBuffer transforms;
	VertexArray vao;
};

ModelFlat* modelCreateFlat(Renderer* r, const char* name,
		size_t numVertices, VertexFlat vertices[])
{
	assert(numVertices);
	assert(vertices);
	ModelFlat* m = alloc(sizeof(*m));
	m->name = name;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFlat) * numVertices, vertices,
			GL_STATIC_DRAW);
	m->numVertices = numVertices;
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->transforms);
	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFlat),
			(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFlat),
			(void*)offsetof(VertexFlat, color));
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

void modelDestroyFlat(Renderer* r, ModelFlat* m)
{
	glDeleteVertexArrays(1, &m->vao);
	glDeleteBuffers(1, &m->transforms);
	glDeleteBuffers(1, &m->tints);
	glDeleteBuffers(1, &m->vertices);
	logDebug(r->log, u8"Model %s destroyed", m->name);
	free(m);
	m = null;
}

void modelDrawFlat(Renderer* r, ModelFlat* m, size_t instances,
		Color4 tints[instances], mat4x4 transforms[instances])
{
	assert(r);
	assert(m);
	assert(m->vao);
	assert(m->name);
	assert(m->vertices);
	assert(m->tints);
	assert(m->transforms);
	assert(tints);
	assert(transforms);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Color4) * instances, null,
			GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Color4) * instances, tints);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, null,
			GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	glUseProgram(r->flat.id);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(r->flat.projection, 1, GL_FALSE, r->projection[0]);
	glUniformMatrix4fv(r->flat.camera, 1, GL_FALSE, r->camera[0]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}
