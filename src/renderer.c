/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath/linmath.h"
#include "window.h"
#include "util.h"
#include "time.h"
#include "log.h"

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
/// @enum ProgramType
typedef enum ProgramType {
	ProgramTypeNone, ///< zero value
	ProgramTypeFlat, ///< Flat shading and no lighting. Most basic shader
	ProgramTypePhong, ///< Phong shading. Simple but makes use of light sources
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
			.name = u8"flat.vert",
			.source = (GLchar[]){
#include "flat.vert"
				, '\0'}},
		.frag = {
			.name = u8"flat.frag",
			.source = (GLchar[]){
#include "flat.frag"
				, '\0'}}
	},
	[ProgramTypePhong] = {
		.vert = {
			.name = u8"phong.vert",
			.source = (GLchar[]){
#include "phong.vert"
				, '\0'}},
		.frag = {
			.name = u8"phong.frag",
			.source = (GLchar[]){
#include "phong.frag"
				, '\0'}}
	}
};

/// Instance of a #ProgramTypeFlat shader program
typedef struct ProgramFlat {
	Program id; ///< ID of the program object
	const ProgramSources* sources; ///< Pointer to shader sources for reference
	Uniform camera; ///< Location of "camera" uniform
	Uniform projection; ///< Location of "projection" uniform
} ProgramFlat;

/// Instance of a #ProgramTypePhong shader program
typedef struct ProgramPhong {
	Program id; ///< ID of the program object
	const ProgramSources* sources; ///< Pointer to shader sources for reference
	Uniform camera; ///< Location of "camera" uniform
	Uniform projection; ///< Location of "projection" uniform
	Uniform lightPosition; ///< Location of "lightPosition" uniform
	Uniform lightColor; ///< Location of "lightColor" uniform
	Uniform ambient; ///< Location of "ambient" uniform
	Uniform diffuse; ///< Location of "diffuse" uniform
	Uniform specular; ///< Location of "specular" uniform
	Uniform shine; ///< Location of "shine" uniform
} ProgramPhong;

typedef struct Renderer {
	Size2i size; ///< Size of the rendering viewport in pixels
	mat4x4 projection; ///< Projection matrix (perspective transform)
	mat4x4 camera; ///< Camera matrix (world transform)
	Point3f lightPosition; ///< Position of light source in world space
	Color3 lightColor; ///< Color of the light source
	ModelFlat* sync; ///< Invisible model used to prevent frame buffering
	ProgramFlat flat; ///< Data of the built-in #ProgramTypeFlat shader
	ProgramPhong phong; ///< Data of the built-in #ProgramTypePhong shader
} Renderer;

/// State of window system initialization
static bool initialized = false;

/// Global renderer instance
Renderer apprenderer = {0};

/**
 * Create an OpenGL shader object. The shader is compiled and ready for linking.
 * @param source Struct containing shader's name and GLSL source code
 * @param type GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @return Newly created ::Shader's ID
 */
static Shader shaderCreate(ShaderSource source, GLenum type)
{
	assert(initialized);
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
		logError(applog, u8"Failed to compile shader %s: %s",
			source.name, infoLog);
		glDeleteShader(shader);
		return 0;
	}
	assert(shader);
	logDebug(applog, u8"Compiled %s shader %s",
		type == GL_VERTEX_SHADER ? u8"vertex" : u8"fragment", source.name);
	return shader;
}

/**
 * Destroy a ::Shader instance. The shader ID becomes invalid and cannot be
 * used again.
 * @param shader ::Shader ID to destroy
 */
static void shaderDestroy(Shader shader)
{
	assert(initialized);
	glDeleteShader(shader);
}

/**
 * Create a ::Program instance by compiling and linking a vertex shader and a
 * fragment shader.
 * @param sources Struct containing vertex and fragment shaders' GLSL sources
 * @return Newly created ::Program's ID
 */
static Program programCreate(ProgramSources sources)
{
	assert(initialized);
	assert(sources.vert.name);
	assert(sources.vert.source);
	assert(sources.frag.name);
	assert(sources.frag.source);
	Shader vert = shaderCreate(sources.vert, GL_VERTEX_SHADER);
	if (vert == 0)
		return 0;
	Shader frag = shaderCreate(sources.frag, GL_FRAGMENT_SHADER);
	if (frag == 0) {
		shaderDestroy(vert); // Proper cleanup, how fancy
		return 0;
	}

	Program program = 0;
	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	GLint linkStatus = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetProgramInfoLog(program, 512, null, infoLog);
		logError(applog, u8"Failed to link shader program %s+%s: %s",
			sources.vert.name, sources.frag.name, infoLog);
		glDeleteProgram(program);
		program = 0;
	}
	shaderDestroy(frag);
	frag = 0;
	shaderDestroy(vert);
	vert = 0;
	logDebug(applog, u8"Linked shader program %s+%s",
		sources.vert.name, sources.frag.name);
	return program;
}

/**
 * Destroy a ::Program instance. The program ID becomes invalid and cannot be
 * used again.
 * @param program ::Program ID to destroy
 */
static void programDestroy(Program program)
{
	assert(initialized);
	glDeleteProgram(program);
	program = 0;
}

static Uniform programUniform(Program program, const char* uniform)
{
	assert(initialized);
	Uniform result = glGetUniformLocation(program, uniform);
	if (result == -1)
		logWarn(applog,
			u8"\"%s\" uniform not available in shader program TODO",
			uniform);
	return result;
}

/**
 * Prevent the driver from buffering commands. Call this after windowFrameEnd()
 * to minimize video latency.
 * @see https://danluu.com/latency-mitigation/
 */
static void rendererSync(void)
{
	assert(initialized);
	mat4x4 identity = {0};
	mat4x4_identity(identity);
	modelDrawFlat(apprenderer.sync, 1,
		(Color4[]){1.0f, 1.0f, 1.0f, 1.0f}, &identity);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, secToNsec(0.1));
}

/**
 * Resize the ::Renderer viewport, preferably to ::Window size. Recreates the
 * matrices as needed.
 * @param size New viewport size in pixels
 */
static void rendererResize(Size2i size)
{
	assert(initialized);
	assert(size.x > 0);
	assert(size.y > 0);
	apprenderer.size.x = size.x;
	apprenderer.size.y = size.y;
	glViewport(0, 0, size.x, size.y);
	mat4x4_perspective(apprenderer.projection, radf(45.0f),
		(float)size.x / (float)size.y, ProjectionNear, ProjectionFar);
}

void rendererInit(void)
{
	if (initialized) return;

	// Pick up the OpenGL context
	windowContextActivate();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit(applog, u8"Failed to initialize OpenGL");
		exit(EXIT_FAILURE);
	}
	initialized = true;

	// Set up global OpenGL state
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);

	// Create built-in shaders
	apprenderer.flat.id = programCreate(DefaultShaders[ProgramTypeFlat]);
	apprenderer.flat.sources = &DefaultShaders[ProgramTypeFlat];
	apprenderer.flat.projection = programUniform(apprenderer.flat.id,
		u8"projection");
	apprenderer.flat.camera = programUniform(apprenderer.flat.id, u8"camera");

	apprenderer.phong.id = programCreate(DefaultShaders[ProgramTypePhong]);
	apprenderer.phong.sources = &DefaultShaders[ProgramTypePhong];
	apprenderer.phong.projection = programUniform(apprenderer.phong.id,
		u8"projection");
	apprenderer.phong.camera = programUniform(apprenderer.phong.id, u8"camera");
	apprenderer.phong.lightPosition = programUniform(apprenderer.phong.id,
		u8"lightPosition");
	apprenderer.phong.lightColor = programUniform(apprenderer.phong.id,
		u8"lightColor");
	apprenderer.phong.ambient = programUniform(apprenderer.phong.id,
		u8"ambient");
	apprenderer.phong.diffuse = programUniform(apprenderer.phong.id,
		u8"diffuse");
	apprenderer.phong.specular = programUniform(apprenderer.phong.id,
		u8"specular");
	apprenderer.phong.shine = programUniform(apprenderer.phong.id, u8"shine");

	// Set up matrices
	rendererResize(windowGetSize());

	// Set up the camera and light source
	vec3 eye = {-4.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(apprenderer.camera, eye, center, up);
	apprenderer.lightPosition.x = -8.0f;
	apprenderer.lightPosition.y = 32.0f;
	apprenderer.lightPosition.z = 16.0f;
	apprenderer.lightColor.r = 1.0f;
	apprenderer.lightColor.g = 1.0f;
	apprenderer.lightColor.b = 1.0f;

	// Create sync model
	apprenderer.sync = modelCreateFlat(u8"sync", 3, (VertexFlat[]){
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

	logDebug(applog, u8"Created renderer for window \"%s\"", windowGetTitle());
}

void rendererCleanup(void)
{
	if (!initialized) return;
	modelDestroyFlat(apprenderer.sync);
	apprenderer.sync = null;
	programDestroy(apprenderer.flat.id);
	apprenderer.flat.id = 0;
	windowContextDeactivate();
	logDebug(applog, u8"Destroyed renderer for window \"%s\"",
		windowGetTitle());
	initialized = false;
}

void rendererClear(Color3 color)
{
	assert(initialized);
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear((uint32_t)GL_COLOR_BUFFER_BIT | (uint32_t)GL_DEPTH_BUFFER_BIT);
}

void rendererFrameBegin(void)
{
	assert(initialized);
	Size2i windowSize = windowGetSize();
	if (apprenderer.size.x != windowSize.x
		|| apprenderer.size.y != windowSize.y)
		rendererResize(windowSize);

	vec3 eye = {sinf(glfwGetTime() / 2) * 4, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(apprenderer.camera, eye, center, up);
//	apprenderer.lightPosition.x = -8.0f;
//	apprenderer.lightPosition.y = 32.0f;
	apprenderer.lightPosition.x = 8.0f * sinf(glfwGetTime() * 6);
	apprenderer.lightPosition.y = 8.0f + 8.0f * sinf(glfwGetTime() * 7);
	apprenderer.lightPosition.z = 16.0f;
	apprenderer.lightColor.r = 1.0f;
	apprenderer.lightColor.g = 1.0f;
	apprenderer.lightColor.b = 1.0f;
}

void rendererFrameEnd(void)
{
	assert(initialized);
	windowFlip();
	rendererSync();
}

////////////////////////////////////////////////////////////////////////////////

/// Semantic rename of OpenGL vertex buffer object ID
typedef GLuint VertexBuffer;

/// Semantic rename of OpenGL vertex array object ID
typedef GLuint VertexArray;

struct ModelFlat {
	const char* name; ///< Human-readable name for reference
	VertexBuffer vertices; ///< VBO with model vertex data
	size_t numVertices; ///< Number of vertices in the model
	VertexBuffer tints; ///< VBO for storing per-draw tint colors
	VertexBuffer transforms; ///< VBO for storing per-draw model matrices
	VertexArray vao; ///< VAO attribute mapping of the model
};

struct ModelPhong {
	const char* name; ///< Human-readable name for reference
	VertexBuffer vertices; ///< VBO with model vertex data
	VertexBuffer normals; ///< VBO with model normals, generated from vertices
	size_t numVertices; ///< Number of vertices in the model
	VertexBuffer tints; ///< VBO for storing per-draw tint colors
	VertexBuffer transforms; ///< VBO for storing per-draw model matrices
	VertexArray vao; ///< VAO attribute mapping of the model
	MaterialPhong material; ///< Struct of Phong lighting model material data
};

static void modelGenerateNormals(size_t numVertices,
	VertexPhong vertices[numVertices], Point3f normalData[numVertices])
{
	assert(initialized);
	assert(numVertices);
	assert(vertices);
	assert(normalData);
	assert(numVertices % 3 == 0);
	for (size_t i = 0; i < numVertices; i += 3) {
		vec3 v0 = {vertices[i + 0].pos.x,
		           vertices[i + 0].pos.y,
		           vertices[i + 0].pos.z};
		vec3 v1 = {vertices[i + 1].pos.x,
		           vertices[i + 1].pos.y,
		           vertices[i + 1].pos.z};
		vec3 v2 = {vertices[i + 2].pos.x,
		           vertices[i + 2].pos.y,
		           vertices[i + 2].pos.z};
		vec3 u = {0};
		vec3 v = {0};
		vec3_sub(u, v1, v0);
		vec3_sub(v, v2, v0);
		vec3 normal = {0};
		vec3_mul_cross(normal, u, v);
		vec3_norm(normal, normal);
		normalData[i + 0].x = normal[0];
		normalData[i + 0].y = normal[1];
		normalData[i + 0].z = normal[2];
		normalData[i + 1].x = normal[0];
		normalData[i + 1].y = normal[1];
		normalData[i + 1].z = normal[2];
		normalData[i + 2].x = normal[0];
		normalData[i + 2].y = normal[1];
		normalData[i + 2].z = normal[2];
	}
}

ModelFlat* modelCreateFlat(const char* name,
	size_t numVertices, VertexFlat vertices[])
{
	assert(initialized);
	assert(numVertices);
	assert(vertices);
	ModelFlat* m = alloc(sizeof(*m));
	m->name = name;
	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFlat) * m->numVertices, vertices,
		GL_STATIC_DRAW);
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
	logDebug(applog, u8"Model %s created", m->name);
	return m;
}

ModelPhong* modelCreatePhong(const char* name,
	size_t numVertices, VertexPhong vertices[], MaterialPhong material)
{
	assert(initialized);
	assert(numVertices);
	assert(vertices);
	ModelPhong* m = alloc(sizeof(*m));
	m->name = name;
	memcpy(&m->material, &material, sizeof(m->material));

	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPhong) * m->numVertices,
		vertices, GL_STATIC_DRAW);
	Point3f normalData[m->numVertices];
	assert(sizeof(normalData) == sizeof(Point3f) * m->numVertices);
	memset(normalData, 0, sizeof(normalData));
	modelGenerateNormals(m->numVertices, vertices, normalData);
	glGenBuffers(1, &m->normals);
	glBindBuffer(GL_ARRAY_BUFFER, m->normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normalData), normalData,
		GL_STATIC_DRAW);
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->transforms);

	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPhong),
		(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPhong),
		(void*)offsetof(VertexPhong, color));
	glBindBuffer(GL_ARRAY_BUFFER, m->normals);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Point3f),
		(void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Color4),
		(void*)0);
	glVertexAttribDivisor(3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)0);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)sizeof(vec4));
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 3));
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);

	logDebug(applog, u8"Model %s created", m->name);
	return m;
}

void modelDestroyFlat(ModelFlat* m)
{
	assert(initialized);
	glDeleteVertexArrays(1, &m->vao);
	m->vao = 0;
	glDeleteBuffers(1, &m->transforms);
	m->transforms = 0;
	glDeleteBuffers(1, &m->tints);
	m->tints = 0;
	glDeleteBuffers(1, &m->vertices);
	m->vertices = 0;
	logDebug(applog, u8"Model %s destroyed", m->name);
	free(m);
	m = null;
}

void modelDestroyPhong(ModelPhong* m)
{
	assert(initialized);
	glDeleteVertexArrays(1, &m->vao);
	m->vao = 0;
	glDeleteBuffers(1, &m->transforms);
	m->transforms = 0;
	glDeleteBuffers(1, &m->tints);
	m->tints = 0;
	glDeleteBuffers(1, &m->normals);
	m->normals = 0;
	glDeleteBuffers(1, &m->vertices);
	m->vertices = 0;
	logDebug(applog, u8"Model %s destroyed", m->name);
	free(m);
	m = null;
}

void modelDrawFlat(ModelFlat* m, size_t instances,
	Color4 tints[instances], mat4x4 transforms[instances])
{
	assert(initialized);
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
	glUseProgram(apprenderer.flat.id);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(apprenderer.flat.projection, 1, GL_FALSE,
		apprenderer.projection[0]);
	glUniformMatrix4fv(apprenderer.flat.camera, 1, GL_FALSE,
		apprenderer.camera[0]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

void modelDrawPhong(ModelPhong* m, size_t instances,
	Color4 tints[instances], mat4x4 transforms[instances])
{
	assert(initialized);
	assert(m);
	assert(m->vao);
	assert(m->name);
	assert(m->vertices);
	assert(m->normals);
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
	glUseProgram(apprenderer.phong.id);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(apprenderer.phong.projection, 1, GL_FALSE,
		apprenderer.projection[0]);
	glUniformMatrix4fv(apprenderer.phong.camera, 1, GL_FALSE,
		apprenderer.camera[0]);
	vec3 lightPosition = {apprenderer.lightPosition.x,
	                      apprenderer.lightPosition.y,
	                      apprenderer.lightPosition.z};
	vec3 lightColor = {apprenderer.lightColor.r,
	                   apprenderer.lightColor.g,
	                   apprenderer.lightColor.b};
	glUniform3fv(apprenderer.phong.lightPosition, 1, lightPosition);
	glUniform3fv(apprenderer.phong.lightColor, 1, lightColor);
	glUniform1f(apprenderer.phong.ambient, m->material.ambient);
	glUniform1f(apprenderer.phong.diffuse, m->material.diffuse);
	glUniform1f(apprenderer.phong.specular, m->material.specular);
	glUniform1f(apprenderer.phong.shine, m->material.shine);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}
