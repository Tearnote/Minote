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

/// Program type for flat shading 
typedef struct ProgramFlat {
	ProgramBase base;
	Uniform camera;
	Uniform projection;
} ProgramFlat;

/// Program type for Phong shading
typedef struct ProgramPhong {
	ProgramBase base;
	Uniform camera;
	Uniform projection;
	Uniform lightPosition;
	Uniform lightColor;
	Uniform ambientColor;
	Uniform ambient;
	Uniform diffuse;
	Uniform specular;
	Uniform shine;
} ProgramPhong;

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

static bool initialized = false;
static size2i viewportSize = {0}; ///< in pixels
static mat4x4 projection = {0}; ///< perspective transform
static mat4x4 camera = {0}; ///< view transform
static point3f lightPosition = {0}; ///< in world space
static color3 lightColor = {0};
static color3 ambientColor = {0};
static Model* sync = null; ///< Invisible model used to prevent frame buffering
static ProgramFlat* flat = null;
static ProgramPhong* phong = null;

/**
 * Prevent the driver from buffering commands. Call this after windowFlip()
 * to minimize video latency.
 * @see https://danluu.com/latency-mitigation/
 */
static void rendererSync(void)
{
	assert(initialized);
	mat4x4 identity = {0};
	mat4x4_identity(identity);
	modelDraw(sync, 1, (color4[]){Color4White}, &identity);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, secToNsec(0.1));
}

/**
 * Resize the rendering viewport, preferably to window size. Recreates the
 * matrices as needed.
 * @param size New viewport size in pixels
 */
static void rendererResize(size2i size)
{
	assert(initialized);
	assert(size.x > 0);
	assert(size.y > 0);
	viewportSize.x = size.x;
	viewportSize.y = size.y;
	glViewport(0, 0, size.x, size.y);
	mat4x4_perspective(projection, radf(45.0f),
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
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);

	// Create built-in shaders
	flat = programCreate(ProgramFlat,
		ProgramFlatVertName, ProgramFlatVertSrc,
		ProgramFlatFragName, ProgramFlatFragSrc);
	flat->projection = programUniform(flat, u8"projection");
	flat->camera = programUniform(flat, u8"camera");

	phong = programCreate(ProgramPhong,
		ProgramPhongVertName, ProgramPhongVertSrc,
		ProgramPhongFragName, ProgramPhongFragSrc);
	phong->projection = programUniform(phong, u8"projection");
	phong->camera = programUniform(phong, u8"camera");
	phong->lightPosition = programUniform(phong, u8"lightPosition");
	phong->lightColor = programUniform(phong, u8"lightColor");
	phong->ambientColor = programUniform(phong, u8"ambientColor");
	phong->ambient = programUniform(phong, u8"ambient");
	phong->diffuse = programUniform(phong, u8"diffuse");
	phong->specular = programUniform(phong, u8"specular");
	phong->shine = programUniform(phong, u8"shine");

	// Set up matrices
	rendererResize(windowGetSize());

	// Set up the camera and light globals
	vec3 eye = {-4.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(camera, eye, center, up);
	lightPosition.x = -8.0f;
	lightPosition.y = 32.0f;
	lightPosition.z = 16.0f;
	lightColor.r = 1.0f;
	lightColor.g = 1.0f;
	lightColor.b = 1.0f;
	ambientColor.r = 1.0f;
	ambientColor.g = 1.0f;
	ambientColor.b = 1.0f;

	// Create sync model
	sync = modelCreateFlat(u8"sync", 3, (VertexFlat[]){
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
	modelDestroy(sync);
	sync = null;
	programDestroy(phong);
	phong = null;
	programDestroy(flat);
	flat = null;
	windowContextDeactivate();
	logDebug(applog, u8"Destroyed renderer for window \"%s\"",
		windowGetTitle());
	initialized = false;
}

void rendererClear(color3 color)
{
	assert(initialized);
	color3Copy(ambientColor, color);
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear((uint32_t)GL_COLOR_BUFFER_BIT
		| (uint32_t)GL_DEPTH_BUFFER_BIT);
}

void rendererFrameBegin(void)
{
	assert(initialized);
	size2i windowSize = windowGetSize();
	if (viewportSize.x != windowSize.x || viewportSize.y != windowSize.y)
		rendererResize(windowSize);

	vec3 eye = {0.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(camera, eye, center, up);
	lightPosition.x = -8.0f;
	lightPosition.y = 32.0f;
	lightPosition.z = 16.0f;
	lightColor.r = 1.0f;
	lightColor.g = 1.0f;
	lightColor.b = 1.0f;
}

void rendererFrameEnd(void)
{
	assert(initialized);
	windowFlip();
	rendererSync();
}

void rendererDepthOnlyBegin(void)
{
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void rendererDepthOnlyEnd(void)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

////////////////////////////////////////////////////////////////////////////////

/// Semantic rename of OpenGL vertex buffer object ID
typedef GLuint VertexBuffer;

/// Semantic rename of OpenGL vertex array object ID
typedef GLuint VertexArray;

/// Type tag for the Model object
typedef enum ModelType {
	ModelTypeNone, ///< zero value
	ModelTypeFlat, ///< ::ModelFlat
	ModelTypePhong, ///< ::ModelPhong
	ModelTypeSize ///< terminator
} ModelType;

/// Base type for a Model. Contains all attributes common to every model type.
typedef struct Model {
	ModelType type; ///< Correct type to cast the Model to
	const char* name; ///< Human-readable name for reference
} ModelBase;

/// Model type with flat shading. Each instance can be tinted.
typedef struct ModelFlat {
	ModelBase base;
	size_t numVertices;
	VertexBuffer vertices; ///< VBO with model vertex data
	VertexBuffer tints; ///< VBO for storing per-draw tint colors
	VertexBuffer highlights; ///< VBO for storing per-draw color highlight colors
	VertexBuffer transforms; ///< VBO for storing per-draw model matrices
	VertexArray vao;
} ModelFlat;

/// Model type with Phong shading. Makes use of light source and material data.
typedef struct ModelPhong {
	ModelBase base;
	size_t numVertices;
	VertexBuffer vertices; ///< VBO with model vertex data
	VertexBuffer normals; ///< VBO with model normals, generated from vertices
	VertexBuffer tints; ///< VBO for storing per-draw tint colors
	VertexBuffer highlights; ///< VBO for storing per-draw color highlight colors
	VertexBuffer transforms; ///< VBO for storing per-draw model matrices
	VertexArray vao;
	MaterialPhong material;
} ModelPhong;

/**
 * Destroy a ::ModelFlat instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelFlat object to destroy
 */
static void modelDestroyFlat(ModelFlat* m)
{
	assert(initialized);
	assert(m);
	glDeleteVertexArrays(1, &m->vao);
	m->vao = 0;
	glDeleteBuffers(1, &m->transforms);
	m->transforms = 0;
	glDeleteBuffers(1, &m->highlights);
	m->highlights = 0;
	glDeleteBuffers(1, &m->tints);
	m->tints = 0;
	glDeleteBuffers(1, &m->vertices);
	m->vertices = 0;
	logDebug(applog, u8"Model %s destroyed", m->base.name);
	free(m);
	m = null;
}

/**
 * Destroy a ::ModelPhong instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelPhong object to destroy
 */
static void modelDestroyPhong(ModelPhong* m)
{
	assert(initialized);
	assert(m);
	glDeleteVertexArrays(1, &m->vao);
	m->vao = 0;
	glDeleteBuffers(1, &m->transforms);
	m->transforms = 0;
	glDeleteBuffers(1, &m->highlights);
	m->highlights = 0;
	glDeleteBuffers(1, &m->tints);
	m->tints = 0;
	glDeleteBuffers(1, &m->normals);
	m->normals = 0;
	glDeleteBuffers(1, &m->vertices);
	m->vertices = 0;
	logDebug(applog, u8"Model %s destroyed", m->base.name);
	free(m);
	m = null;
}

/**
 * Draw a ::ModelFlat on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelFlat object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color blockTintsOpaque for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
static void modelDrawFlat(ModelFlat* m, size_t instances,
	color4 tints[instances], mat4x4 transforms[instances])
{
	assert(initialized);
	assert(m);
	assert(m->base.type == ModelTypeFlat);
	assert(m->vao);
	assert(m->base.name);
	assert(m->vertices);
	assert(m->tints);
	assert(m->transforms);
	assert(tints);
	assert(transforms);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, null,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances, tints);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, null,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	programUse(flat);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(flat->projection, 1, GL_FALSE, projection[0]);
	glUniformMatrix4fv(flat->camera, 1, GL_FALSE, camera[0]);
	glDisableVertexAttribArray(3);
	glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, 0.0f);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Draw a ::ModelPhong on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelPhong object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color blockTintsOpaque for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
static void modelDrawPhong(ModelPhong* m, size_t instances,
	color4 tints[instances], mat4x4 transforms[instances])
{
	assert(initialized);
	assert(m);
	assert(m->base.type == ModelTypePhong);
	assert(m->vao);
	assert(m->base.name);
	assert(m->vertices);
	assert(m->normals);
	assert(m->tints);
	assert(m->transforms);
	assert(tints);
	assert(transforms);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, null,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances, tints);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, null,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	programUse(phong);
	glBindVertexArray(m->vao);
	glUniformMatrix4fv(phong->projection, 1, GL_FALSE, projection[0]);
	glUniformMatrix4fv(phong->camera, 1, GL_FALSE, camera[0]);
	glUniform3fv(phong->lightPosition, 1, lightPosition.arr);
	glUniform3fv(phong->lightColor, 1, lightColor.arr);
	glUniform3fv(phong->ambientColor, 1, ambientColor.arr);
	glUniform1f(phong->ambient, m->material.ambient);
	glUniform1f(phong->diffuse, m->material.diffuse);
	glUniform1f(phong->specular, m->material.specular);
	glUniform1f(phong->shine, m->material.shine);
	glDisableVertexAttribArray(4);
	glVertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Generate normal vectors for a given triangle mesh.
 * @param numVertices Number of vertices in @a vertices and @a normalData
 * @param vertices Model mesh made out of triangles
 * @param[out] normalData Normal vectors corresponding to the vertex mesh
 */
static void modelGenerateNormals(size_t numVertices,
	VertexPhong vertices[numVertices], point3f normalData[numVertices])
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

Model* modelCreateFlat(const char* name,
	size_t numVertices, VertexFlat vertices[])
{
	assert(initialized);
	assert(name);
	assert(numVertices);
	assert(vertices);
	ModelFlat* m = alloc(sizeof(*m));
	m->base.type = ModelTypeFlat;
	m->base.name = name;
	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFlat) * m->numVertices, vertices,
		GL_STATIC_DRAW);
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->highlights);
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
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
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
	logDebug(applog, u8"Model %s created", m->base.name);
	return (Model*)m;
}

Model* modelCreatePhong(const char* name,
	size_t numVertices, VertexPhong vertices[], MaterialPhong material)
{
	assert(initialized);
	assert(name);
	assert(numVertices);
	assert(vertices);
	ModelPhong* m = alloc(sizeof(*m));
	m->base.type = ModelTypePhong;
	m->base.name = name;
	structCopy(m->material, material);

	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPhong) * m->numVertices,
		vertices, GL_STATIC_DRAW);
	point3f normalData[m->numVertices];
	assert(sizeof(normalData) == sizeof(point3f) * m->numVertices);
	arrayClear(normalData);
	modelGenerateNormals(m->numVertices, vertices, normalData);
	glGenBuffers(1, &m->normals);
	glBindBuffer(GL_ARRAY_BUFFER, m->normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normalData), normalData,
		GL_STATIC_DRAW);
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->highlights);
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
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(point3f),
		(void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(4, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)0);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)sizeof(vec4));
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 3));
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	glVertexAttribDivisor(8, 1);

	logDebug(applog, u8"Model %s created", m->base.name);
	return (Model*)m;
}

void modelDestroy(Model* m)
{
	assert(m);
	assert(m->type > ModelTypeNone && m->type < ModelTypeSize);
	switch (m->type) {
	case ModelTypeFlat:
		modelDestroyFlat((ModelFlat*)m);
		break;
	case ModelTypePhong:
		modelDestroyPhong((ModelPhong*)m);
		break;
	default:
		assert(false);
	}
}

void modelDraw(Model* m, size_t instances,
	color4 tints[instances], mat4x4 transforms[instances])
{
	assert(m);
	assert(m->type > ModelTypeNone && m->type < ModelTypeSize);
	switch (m->type) {
	case ModelTypeFlat:
		modelDrawFlat((ModelFlat*)m, instances, tints, transforms);
		break;
	case ModelTypePhong:
		modelDrawPhong((ModelPhong*)m, instances, tints, transforms);
		break;
	default:
		assert(false);
	}
}
