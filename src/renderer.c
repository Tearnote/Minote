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
#include "shader.h"
#include "util.h"
#include "time.h"
#include "log.h"

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

/// Instance of a #ProgramTypeFlat shader program
typedef struct ProgramFlat {
	ProgramCommon common;
	Uniform camera; ///< Location of "camera" uniform
	Uniform projection; ///< Location of "projection" uniform
} ProgramFlat;

/// Instance of a #ProgramTypePhong shader program
typedef struct ProgramPhong {
	ProgramCommon common;
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
	ProgramFlat* flat; ///< The built-in flat shader
	ProgramPhong* phong; ///< The built-in Phong shader
} Renderer;

static const char* ProgramFlatVertName = u8"flat.vert";
static const GLchar* ProgramFlatVertSrc = (GLchar[]){
#include "flat.vert"
	, '\0'};
static const char* ProgramFlatFragName = u8"flat.frag";
static const GLchar* ProgramFlatFragSrc = (GLchar[]){
#include "flat.frag"
	, '\0'};

static const char* ProgramPhongVertName = u8"phong.vert";
static const GLchar* ProgramPhongVertSrc = (GLchar[]){
#include "phong.vert"
	, '\0'};
static const char* ProgramPhongFragName = u8"phong.frag";
static const GLchar* ProgramPhongFragSrc = (GLchar[]){
#include "phong.frag"
	, '\0'};

/// State of window system initialization
static bool initialized = false;

/// Global renderer instance
Renderer renderer = {0};

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
	modelDrawFlat(renderer.sync, 1, (Color4[]){Color4White}, &identity);
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
	renderer.size.x = size.x;
	renderer.size.y = size.y;
	glViewport(0, 0, size.x, size.y);
	mat4x4_perspective(renderer.projection, radf(45.0f),
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
	renderer.flat = programCreate(ProgramFlat,
		ProgramFlatVertName, ProgramFlatVertSrc,
		ProgramFlatFragName, ProgramFlatFragSrc);
	renderer.flat->projection = programUniform(renderer.flat, u8"projection");
	renderer.flat->camera = programUniform(renderer.flat, u8"camera");

	renderer.phong = programCreate(ProgramPhong,
		ProgramPhongVertName, ProgramPhongVertSrc,
		ProgramPhongFragName, ProgramPhongFragSrc);
	renderer.phong->projection = programUniform(renderer.phong, u8"projection");
	renderer.phong->camera = programUniform(renderer.phong, u8"camera");
	renderer.phong->lightPosition = programUniform(renderer.phong,
		u8"lightPosition");
	renderer.phong->lightColor = programUniform(renderer.phong, u8"lightColor");
	renderer.phong->ambient = programUniform(renderer.phong, u8"ambient");
	renderer.phong->diffuse = programUniform(renderer.phong, u8"diffuse");
	renderer.phong->specular = programUniform(renderer.phong, u8"specular");
	renderer.phong->shine = programUniform(renderer.phong, u8"shine");

	// Set up matrices
	rendererResize(windowGetSize());

	// Set up the camera and light source
	vec3 eye = {-4.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(renderer.camera, eye, center, up);
	renderer.lightPosition.x = -8.0f;
	renderer.lightPosition.y = 32.0f;
	renderer.lightPosition.z = 16.0f;
	renderer.lightColor.r = 1.0f;
	renderer.lightColor.g = 1.0f;
	renderer.lightColor.b = 1.0f;

	// Create sync model
	renderer.sync = modelCreateFlat(u8"sync", 3, (VertexFlat[]){
		{
			.pos = {0.0f, 0.0f, 0.0f},
			.color = Color4Clear
		},
		{
			.pos = {1.0f, 0.0f, 0.0f},
			.color = Color4Clear
		},
		{
			.pos = {0.0f, 1.0f, 0.0f},
			.color = Color4Clear
		}
	});

	logDebug(applog, u8"Created renderer for window \"%s\"", windowGetTitle());
}

void rendererCleanup(void)
{
	if (!initialized) return;
	modelDestroyFlat(renderer.sync);
	renderer.sync = null;
	programDestroy(renderer.phong);
	renderer.phong = null;
	programDestroy(renderer.flat);
	renderer.flat = null;
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
	if (renderer.size.x != windowSize.x || renderer.size.y != windowSize.y)
		rendererResize(windowSize);

	vec3 eye = {sinf(glfwGetTime() / 2) * 4, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(renderer.camera, eye, center, up);
//	apprenderer.lightPosition.x = -8.0f;
//	apprenderer.lightPosition.y = 32.0f;
	renderer.lightPosition.x = 8.0f * sinf(glfwGetTime() * 6);
	renderer.lightPosition.y = 8.0f + 8.0f * sinf(glfwGetTime() * 7);
	renderer.lightPosition.z = 16.0f;
	renderer.lightColor.r = 1.0f;
	renderer.lightColor.g = 1.0f;
	renderer.lightColor.b = 1.0f;
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

/**
 * Generate normal vectors for a given triangle mesh.
 * @param numVertices Number of vertices in @a vertices and @a normalData
 * @param vertices Model mesh made out of triangles
 * @param[out] normalData Normal vectors corresponding to the vertex mesh
 */
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
	programUse(renderer.flat);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(renderer.flat->projection, 1, GL_FALSE,
		renderer.projection[0]);
	glUniformMatrix4fv(renderer.flat->camera, 1, GL_FALSE,
		renderer.camera[0]);
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
	programUse(renderer.phong);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(renderer.phong->projection, 1, GL_FALSE,
		renderer.projection[0]);
	glUniformMatrix4fv(renderer.phong->camera, 1, GL_FALSE,
		renderer.camera[0]);
	vec3 lightPosition = {renderer.lightPosition.x,
	                      renderer.lightPosition.y,
	                      renderer.lightPosition.z};
	vec3 lightColor = {renderer.lightColor.r,
	                   renderer.lightColor.g,
	                   renderer.lightColor.b};
	glUniform3fv(renderer.phong->lightPosition, 1, lightPosition);
	glUniform3fv(renderer.phong->lightColor, 1, lightColor);
	glUniform1f(renderer.phong->ambient, m->material.ambient);
	glUniform1f(renderer.phong->diffuse, m->material.diffuse);
	glUniform1f(renderer.phong->specular, m->material.specular);
	glUniform1f(renderer.phong->shine, m->material.shine);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}
