#include "engine/model.hpp"

namespace minote {

// Generate normals in-place for an array of vertices.
static void generatePhongNormals(span<ModelPhong::Vertex> const vertices)
{
	DASSERT(vertices.size() % 3 == 0);

	for (size_t i = 0; i < vertices.size(); i += 3) {
		vec3 const& v0 = vertices[i + 0].pos;
		vec3 const& v1 = vertices[i + 1].pos;
		vec3 const& v2 = vertices[i + 2].pos;

		vec3 const normal = normalize(cross(v1 - v0, v2 - v0));
		vertices[i + 0].normal = normal;
		vertices[i + 1].normal = normal;
		vertices[i + 2].normal = normal;
	}
}

void ModelFlat::create(char const* _name, Shaders& shaders,
	span<Vertex const> const _vertices)
{
	DASSERT(_name);
	DASSERT(_vertices.size() % 3 == 0);

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
	drawcall.triangles = _vertices.size() / 3;

	name = _name;
	L.debug(R"(Model "{}" created)", name);
}

void ModelFlat::destroy()
{
	DASSERT(vertices.id);

	vertices.destroy();
	instances.destroy();
	vao.destroy();
	drawcall = {};

	L.debug(R"(Model "{}" destroyed)", name);
	name = nullptr;
}

void ModelFlat::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params)
{
	draw(fb, scene, params, Instance{});
}

void ModelFlat::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, Instance const& instance)
{
	DASSERT(vertices.id);

	instances.upload(array{instance});
	drawcall.shader->view = scene.view;
	drawcall.shader->projection = scene.projection;
	drawcall.framebuffer = &fb;
	drawcall.instances = 1;
	drawcall.params = params;
	drawcall.draw();
}

void ModelFlat::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, span<Instance const> const _instances)
{
	DASSERT(vertices.id);

	instances.upload(_instances);
	drawcall.shader->view = scene.view;
	drawcall.shader->projection = scene.projection;
	drawcall.framebuffer = &fb;
	drawcall.instances = _instances.size();
	drawcall.params = params;
	drawcall.draw();
}

void ModelPhong::create(char const* _name, Shaders& shaders,
	span<Vertex const> const _vertices, Material _material,
	bool const generateNormals)
{
	DASSERT(_name);
	DASSERT(_vertices.size() % 3 == 0);

	vertices.create("Phong::vertices", false);
	if (generateNormals) {
		auto normVertices = dvector<Vertex>{_vertices.begin(), _vertices.end()};
		generatePhongNormals(normVertices);
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
	drawcall.triangles = _vertices.size() / 3;

	name = _name;
	L.debug(R"(Model "{}" created)", name);
}

void ModelPhong::destroy()
{
	DASSERT(vertices.id);

	vertices.destroy();
	instances.destroy();
	vao.destroy();
	drawcall = {};

	L.debug(R"(Model "{}" destroyed)", name);
	name = nullptr;
}

void ModelPhong::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params)
{
	draw(fb, scene, params, Instance{});
}

void ModelPhong::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, Instance const& instance)
{
	DASSERT(vertices.id);

	instances.upload(array{instance});
	drawcall.shader->view = scene.view;
	drawcall.shader->projection = scene.projection;
	drawcall.shader->lightPosition = scene.light.position;
	drawcall.shader->lightColor = scene.light.color;
	drawcall.shader->ambient = material.ambient;
	drawcall.shader->diffuse = material.diffuse;
	drawcall.shader->specular = material.specular;
	drawcall.shader->shine = material.shine;
	drawcall.framebuffer = &fb;
	drawcall.instances = 1;
	drawcall.params = params;
	drawcall.draw();
}

void ModelPhong::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, span<Instance const> const _instances)
{
		DASSERT(vertices.id);

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
	drawcall.instances = _instances.size();
	drawcall.params = params;
	drawcall.draw();
}

}
