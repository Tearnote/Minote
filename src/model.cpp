/**
 * Implementation of model.h
 * @file
 */

#include "model.hpp"

#include "world.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

using namespace minote;

/// Flat shading type
typedef struct ProgramFlat {
	ProgramBase base;
	Uniform camera;
	Uniform projection;
} ProgramFlat;

/// Phong-Blinn shading type
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

static const char* ProgramFlatVertName = "flat.vert";
static const GLchar* ProgramFlatVertSrc = (GLchar[]){
#include "flat.vert"
	'\0'};
static const char* ProgramFlatFragName = "flat.frag";
static const GLchar* ProgramFlatFragSrc = (GLchar[]){
#include "flat.frag"
	'\0'};

static const char* ProgramPhongVertName = "phong.vert";
static const GLchar* ProgramPhongVertSrc = (GLchar[]){
#include "phong.vert"
	'\0'};
static const char* ProgramPhongFragName = "phong.frag";
static const GLchar* ProgramPhongFragSrc = (GLchar[]){
#include "phong.frag"
	'\0'};

static ProgramFlat* flat = nullptr;
static ProgramPhong* phong = nullptr;

static bool initialized = false;

void modelInit(void)
{
	if (initialized) return;

	flat = programCreate(ProgramFlat,
		ProgramFlatVertName, ProgramFlatVertSrc,
		ProgramFlatFragName, ProgramFlatFragSrc);
	flat->projection = programUniform(flat, "projection");
	flat->camera = programUniform(flat, "camera");

	phong = programCreate(ProgramPhong,
		ProgramPhongVertName, ProgramPhongVertSrc,
		ProgramPhongFragName, ProgramPhongFragSrc);
	phong->projection = programUniform(phong, "projection");
	phong->camera = programUniform(phong, "camera");
	phong->lightPosition = programUniform(phong, "lightPosition");
	phong->lightColor = programUniform(phong, "lightColor");
	phong->ambientColor = programUniform(phong, "ambientColor");
	phong->ambient = programUniform(phong, "ambient");
	phong->diffuse = programUniform(phong, "diffuse");
	phong->specular = programUniform(phong, "specular");
	phong->shine = programUniform(phong, "shine");

	initialized = true;
}

void modelCleanup(void)
{
	if (!initialized) return;

	programDestroy(phong);
	phong = nullptr;
	programDestroy(flat);
	flat = nullptr;

	initialized = false;
}

/**
 * Destroy a ::ModelFlat instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelFlat object to destroy
 */
static void modelDestroyFlat(ModelFlat* m)
{
	ASSERT(m);
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
	L.debug("Model %s destroyed", m->base.name);
	free(m);
	m = nullptr;
}

/**
 * Destroy a ::ModelPhong instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelPhong object to destroy
 */
static void modelDestroyPhong(ModelPhong* m)
{
	ASSERT(m);
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
	L.debug("Model %s destroyed", m->base.name);
	free(m);
	m = nullptr;
}

/**
 * Draw a ::ModelFlat on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelFlat object to draw
 * @param instances Number of instances to draw
 * @param tints Color tints for each instance. Can be null.
 * @param highlights Highlight colors to blend into each instance. Can be null
 * @param transforms 4x4 matrices for transforming each instance
 */
static void modelDrawFlat(ModelFlat* m, size_t instances,
	color4 tints[], color4 highlights[],
	mat4x4 transforms[])
{
	ASSERT(initialized);
	ASSERT(m);
	ASSERT(m->base.type == ModelTypeFlat);
	ASSERT(m->vao);
	ASSERT(m->base.name);
	ASSERT(m->vertices);
	ASSERT(m->tints);
	ASSERT(m->transforms);
	ASSERT(transforms);
	if (!instances) return;

	glBindVertexArray(m->vao);
	programUse(flat);
	if (tints) {
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, m->tints);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, nullptr,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances, tints);
	} else {
		glDisableVertexAttribArray(2);
		glVertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (highlights) {
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, nullptr,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances,
			highlights);
	} else {
		glDisableVertexAttribArray(3);
		glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, nullptr,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	glUniformMatrix4fv(flat->projection, 1, GL_FALSE, *worldProjection);
	glUniformMatrix4fv(flat->camera, 1, GL_FALSE, *worldCamera);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Draw a ::ModelPhong on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelPhong object to draw
 * @param instances Number of instances to draw
 * @param tints Color tints for each instance. Can be null
 * @param highlights Highlight colors to blend into each instance. Can be null
 * @param transforms 4x4 matrices for transforming each instance
 */
static void modelDrawPhong(ModelPhong* m, size_t instances,
	color4 tints[], color4 highlights[],
	mat4x4 transforms[])
{
	ASSERT(initialized);
	ASSERT(m);
	ASSERT(m->base.type == ModelTypePhong);
	ASSERT(m->vao);
	ASSERT(m->base.name);
	ASSERT(m->vertices);
	ASSERT(m->normals);
	ASSERT(m->tints);
	ASSERT(m->transforms);
	ASSERT(transforms);
	if (!instances) return;

	glBindVertexArray(m->vao);
	programUse(phong);
	if (tints) {
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, m->tints);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, nullptr,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances, tints);
	} else {
		glDisableVertexAttribArray(3);
		glVertexAttrib4f(3, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (highlights) {
		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, nullptr,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances,
			highlights);
	} else {
		glDisableVertexAttribArray(4);
		glVertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, nullptr,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	glUniformMatrix4fv(phong->projection, 1, GL_FALSE, *worldProjection);
	glUniformMatrix4fv(phong->camera, 1, GL_FALSE, *worldCamera);
	glUniform3fv(phong->lightPosition, 1, reinterpret_cast<GLfloat*>(&worldLightPosition));
	glUniform3fv(phong->lightColor, 1, reinterpret_cast<GLfloat*>(&worldLightColor));
	glUniform3fv(phong->ambientColor, 1, reinterpret_cast<GLfloat*>(&worldAmbientColor));
	glUniform1f(phong->ambient, m->material.ambient);
	glUniform1f(phong->diffuse, m->material.diffuse);
	glUniform1f(phong->specular, m->material.specular);
	glUniform1f(phong->shine, m->material.shine);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Generate normal vectors for a given triangle mesh.
 * @param numVertices Number of vertices in @a vertices and @a normalData
 * @param vertices Model mesh made out of triangles
 * @param[out] normalData Normal vectors corresponding to the vertex mesh
 */
static void modelGenerateNormals(size_t numVertices,
	VertexPhong vertices[], point3f normalData[])
{
	ASSERT(numVertices);
	ASSERT(vertices);
	ASSERT(normalData);
	ASSERT(numVertices % 3 == 0);
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
	ASSERT(name);
	ASSERT(numVertices);
	ASSERT(vertices);
	auto* m = allocate<ModelFlat>();
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
	L.debug("Model %s created", m->base.name);
	return (Model*)m;
}

Model* modelCreatePhong(const char* name,
	size_t numVertices, VertexPhong vertices[],
	MaterialPhong material)
{
	ASSERT(name);
	ASSERT(numVertices);
	ASSERT(vertices);
	auto* m = allocate<ModelPhong>();
	m->base.type = ModelTypePhong;
	m->base.name = name;
	m->material = material;

	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPhong) * m->numVertices,
		vertices, GL_STATIC_DRAW);
	point3f normalData[m->numVertices];
	ASSERT(sizeof(normalData) == sizeof(point3f) * m->numVertices);
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

	L.debug("Model %s created", m->base.name);
	return (Model*)m;
}

void modelDestroy(Model* m)
{
	if (!m) return;
	ASSERT(m->type > ModelTypeNone && m->type < ModelTypeSize);
	switch (m->type) {
	case ModelTypeFlat:
		modelDestroyFlat((ModelFlat*)m);
		break;
	case ModelTypePhong:
		modelDestroyPhong((ModelPhong*)m);
		break;
	default:
		ASSERT(false);
	}
}

void modelDraw(Model* m, size_t instances,
	color4 tints[], color4 highlights[],
	mat4x4 transforms[])
{
	ASSERT(m);
	ASSERT(m->type > ModelTypeNone && m->type < ModelTypeSize);
	ASSERT(transforms);

	switch (m->type) {
	case ModelTypeFlat:
		modelDrawFlat((ModelFlat*)m, instances, tints, highlights, transforms);
		break;
	case ModelTypePhong:
		modelDrawPhong((ModelPhong*)m, instances, tints, highlights,
			transforms);
		break;
	default:
		ASSERT(false);
	}
}
