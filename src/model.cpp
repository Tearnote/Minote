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
struct Flat : Shader {

	Uniform<mat4> camera;
	Uniform<mat4> projection;

	void setLocations() override
	{
		camera.setLocation(*this, "camera");
		projection.setLocation(*this, "projection");
	}

};

/// Phong-Blinn shading type
struct Phong : Shader {

	Uniform<mat4> camera;
	Uniform<mat4> projection;
	Uniform<vec3> lightPosition;
	Uniform<vec3> lightColor;
	Uniform<vec3> ambientColor;
	Uniform<float> ambient;
	Uniform<float> diffuse;
	Uniform<float> specular;
	Uniform<float> shine;

	void setLocations() override
	{
		camera.setLocation(*this, "camera");
		projection.setLocation(*this, "projection");
		lightPosition.setLocation(*this, "lightPosition");
		lightColor.setLocation(*this, "lightColor");
		ambientColor.setLocation(*this, "ambientColor");
		ambient.setLocation(*this, "ambient");
		diffuse.setLocation(*this, "diffuse");
		specular.setLocation(*this, "specular");
		shine.setLocation(*this, "shine");
	}

};

static const GLchar* FlatVertSrc = (GLchar[]){
#include "flat.vert"
	'\0'};
static const GLchar* FlatFragSrc = (GLchar[]){
#include "flat.frag"
	'\0'};

static const GLchar* PhongVertSrc = (GLchar[]){
#include "phong.vert"
	'\0'};
static const GLchar* PhongFragSrc = (GLchar[]){
#include "phong.frag"
	'\0'};

static Flat flat;
static Phong phong;

static bool initialized = false;

void modelInit(void)
{
	if (initialized) return;

	flat.create("flat", FlatVertSrc, FlatFragSrc);
	phong.create("phong", PhongVertSrc, PhongFragSrc);

	initialized = true;
}

void modelCleanup(void)
{
	if (!initialized) return;

	phong.destroy();
	flat.destroy();

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
	m->transforms.destroy();
	m->highlights.destroy();
	m->tints.destroy();
	m->vertices.destroy();
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
	m->transforms.destroy();
	m->highlights.destroy();
	m->tints.destroy();
	m->normals.destroy();
	m->vertices.destroy();
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
	mat4 transforms[])
{
	ASSERT(initialized);
	ASSERT(m);
	ASSERT(m->base.type == ModelTypeFlat);
	ASSERT(m->vao);
	ASSERT(m->base.name);
	ASSERT(transforms);
	if (!instances) return;

	glBindVertexArray(m->vao);
	flat.bind();
	if (tints) {
		glEnableVertexAttribArray(2);
		m->tints.upload(instances, tints);
	} else {
		glDisableVertexAttribArray(2);
		glVertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (highlights) {
		glEnableVertexAttribArray(3);
		m->highlights.upload(instances, highlights);
	} else {
		glDisableVertexAttribArray(3);
		glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	m->transforms.upload(instances, transforms);
	flat.projection = worldProjection;
	flat.camera = worldCamera;
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
	mat4 transforms[])
{
	ASSERT(initialized);
	ASSERT(m);
	ASSERT(m->base.type == ModelTypePhong);
	ASSERT(m->vao);
	ASSERT(m->base.name);
	ASSERT(transforms);
	if (!instances) return;

	glBindVertexArray(m->vao);
	phong.bind();
	if (tints) {
		glEnableVertexAttribArray(3);
		m->tints.upload(instances, tints);
	} else {
		glDisableVertexAttribArray(3);
		glVertexAttrib4f(3, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (highlights) {
		glEnableVertexAttribArray(4);
		m->highlights.upload(instances, highlights);
	} else {
		glDisableVertexAttribArray(4);
		glVertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	m->transforms.upload(instances, transforms);
	phong.projection = worldProjection;
	phong.camera = worldCamera;
	phong.lightPosition = worldLightPosition;
	phong.lightColor = worldLightColor;
	phong.ambientColor = worldAmbientColor;
	phong.ambient = m->material.ambient;
	phong.diffuse = m->material.diffuse;
	phong.specular = m->material.specular;
	phong.shine = m->material.shine;
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Generate normal vectors for a given triangle mesh.
 * @param numVertices Number of vertices in @a vertices and @a normalData
 * @param vertices Model mesh made out of triangles
 * @param[out] normalData Normal vectors corresponding to the vertex mesh
 */
static void modelGenerateNormals(size_t numVertices,
	VertexPhong vertices[], vec3 normalData[])
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
		vec3 normal = normalize(cross(v1 - v0, v2 - v0));
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
	m->vertices.create("vertices", false);
	m->vertices.upload(m->numVertices, vertices);
	m->tints.create("tints", true);
	m->highlights.create("highlights", true);
	m->transforms.create("transforms", true);

	m->vertices.bind();
	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFlat),
		(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFlat),
		(void*)offsetof(VertexFlat, color));
	m->tints.bind();
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(2, 1);
	m->highlights.bind();
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(3, 1);
	m->transforms.bind();
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
		(void*)0);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
		(void*)sizeof(vec4));
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
		(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
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
	m->vertices.create("vertices", false);
	m->vertices.upload(m->numVertices, vertices);
	vec3 normalData[m->numVertices];
	ASSERT(sizeof(normalData) == sizeof(vec3) * m->numVertices);
	arrayClear(normalData);
	modelGenerateNormals(m->numVertices, vertices, normalData);
	m->normals.create("normals", false);
	m->normals.upload(m->numVertices, normalData);
	m->tints.create("tints", true);
	m->highlights.create("highlights", true);
	m->transforms.create("transforms", true);

	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	m->vertices.bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPhong),
		(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPhong),
		(void*)offsetof(VertexPhong, color));
	m->normals.bind();
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3),
		(void*)0);
	m->tints.bind();
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(3, 1);
	m->highlights.bind();
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(4, 1);
	m->transforms.bind();
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
		(void*)0);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
		(void*)sizeof(vec4));
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
		(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(mat4),
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
	mat4 transforms[])
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
