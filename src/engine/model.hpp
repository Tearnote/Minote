/**
 * Drawing of 3D mesh data with a number of shading models
 * @file
 */

#pragma once

#include "sys/opengl.hpp"
#include "engine/scene.hpp"
#include "store/shaders.hpp"

namespace minote {

struct ModelFlat {

	struct Vertex {

		vec3 pos = {0.0f, 0.0f, 0.0f};
		color4 color = {1.0f, 1.0f, 1.0f, 1.0f};

	};

	struct Instance {

		color4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
		color4 highlight = {0.0f, 0.0f, 0.0f, 0.0f};
		mat4 transform = mat4(1.0f);

	};

	char const* name = nullptr;
	VertexBuffer<Vertex> vertices;
	VertexBuffer<Instance> instances;
	VertexArray vao;
	Draw<Shaders::Flat> drawcall;

	template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
		requires ArrayContainer<Arr, Vertex, N>
	void create(char const* name, Shaders& shaders,
		Arr<Vertex, N> const& vertices);

	void destroy();

	void draw(Framebuffer& fb, Scene const& scene, DrawParams const& params);

	void draw(Framebuffer& fb, Scene const& scene, DrawParams const& params,
		Instance const& instance);

	template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
		requires ArrayContainer<Arr, Instance, N>
	void draw(Framebuffer& fb, Scene const& scene, DrawParams const& params,
		Arr<Instance, N> const& instances);

};

struct ModelPhong {

	struct Vertex {

		vec3 pos = {0.0f, 0.0f, 0.0f};
		color4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		vec3 normal = {0.0f, 1.0f, 0.0f};

	};

	struct Instance {

		color4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
		color4 highlight = {0.0f, 0.0f, 0.0f, 0.0f};
		mat4 transform = mat4(1.0f);

	};

	struct Material {

		f32 ambient = 0.0f;
		f32 diffuse = 0.0f;
		f32 specular = 0.0f;
		f32 shine = 1.0f; ///< Smoothness of surface (inverse of specular highlight size)

	};

	char const* name = nullptr;
	VertexBuffer<Vertex> vertices;
	VertexBuffer<Instance> instances;
	Material material;
	VertexArray vao;
	Draw<Shaders::Phong> drawcall;

	template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, Vertex, N>
	void create(char const* name, Shaders& shaders,
		Arr<Vertex, N> const& vertices, Material material, bool generateNormals = false);

	void destroy();

	void draw(Framebuffer& fb, Scene const& scene, DrawParams const& params);

	void draw(Framebuffer& fb, Scene const& scene, DrawParams const& params,
		Instance const& instance);

	template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, Instance, N>
	void draw(Framebuffer& fb, Scene const& scene, DrawParams const& params,
		Arr<Instance, N> const& instances);

};

}

#include "model.tpp"
