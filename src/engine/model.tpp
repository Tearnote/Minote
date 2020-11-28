#include "engine/model.hpp"

namespace minote {

namespace detail {

// Generate normals in-place for an array of vertices.
template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, ModelPhong::Vertex, N>
void generatePhongNormals(Arr<ModelPhong::Vertex, N>& vertices)
{
	static_assert(N % 3 == 0);

	for (size_t i = 0; i < N; i += 3) {
		vec3 const& v0 = vertices[i + 0].pos;
		vec3 const& v1 = vertices[i + 1].pos;
		vec3 const& v2 = vertices[i + 2].pos;

		vec3 const normal = normalize(cross(v1 - v0, v2 - v0));
		vertices[i + 0].normal = normal;
		vertices[i + 1].normal = normal;
		vertices[i + 2].normal = normal;
	}
}

}

template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, ModelFlat::Vertex, N>
void ModelFlat::create(char const* _name, Shaders& shaders,
		Arr<Vertex, N> const& _vertices)
{
	ASSERT(_name);
	static_assert(N % 3 == 0);

	vertices.create("Flat::vertices", false);
	vertices.upload(_vertices);
	instances.create("Flat::instances", true);
	vao.create("Flat::vao");
	vao.setAttribute(0, vertices, &Vertex::pos);
	vao.setAttribute(1, vertices, &Vertex::color);
	vao.setAttribute(2, instances, &Instance::tint, true);
	vao.setAttribute(3, instances, &Instance::highlight, true);
	vao.setAttribute(4, instances, &Instance::transform, true);
	drawcall.shader = &shaders.flat;
	drawcall.vertexarray = &vao;
	drawcall.triangles = N / 3;

	name = _name;
	L.debug(R"(Model "{}" created)", name);
}

template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, ModelFlat::Instance, N>
void ModelFlat::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, Arr<Instance, N> const& _instances)
{
	ASSERT(vertices.id);

	instances.upload(_instances);
	drawcall.shader->view = scene.view;
	drawcall.shader->projection = scene.projection;
	drawcall.framebuffer = &fb;
	drawcall.instances = _instances.size();
	drawcall.params = params;
	drawcall.draw();
}

template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, ModelPhong::Vertex, N>
void ModelPhong::create(char const* _name, Shaders& shaders,
		Arr<Vertex, N> const& _vertices, Material _material,
		bool const generateNormals)
{
	ASSERT(_name);
	static_assert(N % 3 == 0);

	vertices.create("Phong::vertices", false);
	if (generateNormals) {
		auto normVertices = _vertices;
		detail::generatePhongNormals(normVertices);
		vertices.upload(normVertices);
	} else {
		vertices.upload(_vertices);
	}

	instances.create("Phong::instances", true);
	material = _material;
	vao.create("Phong::vao");
	vao.setAttribute(0, vertices, &Vertex::pos);
	vao.setAttribute(1, vertices, &Vertex::color);
	vao.setAttribute(2, vertices, &Vertex::normal);
	vao.setAttribute(3, instances, &Instance::tint, true);
	vao.setAttribute(4, instances, &Instance::highlight, true);
	vao.setAttribute(5, instances, &Instance::transform, true);
	drawcall.shader = &shaders.phong;
	drawcall.vertexarray = &vao;
	drawcall.triangles = N / 3;

	name = _name;
	L.debug(R"(Model "{}" created)", name);
}

template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, ModelPhong::Instance, N>
void ModelPhong::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, Arr<Instance, N> const& _instances)
{
	ASSERT(vertices.id);

	instances.upload(_instances);
	drawcall.shader->view = scene.view;
	drawcall.shader->projection = scene.projection;
	drawcall.shader->lightPosition = scene.light.position;
	drawcall.shader->lightColor = scene.light.color;
	drawcall.shader->ambient = material.ambient;
	drawcall.shader->diffuse = material.diffuse;
	drawcall.shader->specular = material.specular;
	drawcall.shader->shine = material.shine;
	drawcall.framebuffer = &fb;
	drawcall.instances = N;
	drawcall.params = params;
	drawcall.draw();
}

}
