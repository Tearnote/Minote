#include "engine/model.hpp"

namespace minote {

void ModelFlat::destroy()
{
	ASSERT(vertices.id);

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
	draw(fb, scene, params, {});
}

void ModelFlat::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, Instance const& instance)
{
	ASSERT(vertices.id);

	instances.upload(array{instance});
	drawcall.shader->view = scene.view;
	drawcall.shader->projection = scene.projection;
	drawcall.framebuffer = &fb;
	drawcall.instances = 1;
	drawcall.params = params;
	drawcall.draw();
}

void ModelPhong::destroy()
{
	ASSERT(vertices.id);

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
	draw(fb, scene, params, {});
}

void ModelPhong::draw(Framebuffer& fb, Scene const& scene,
	DrawParams const& params, Instance const& instance)
{
	ASSERT(vertices.id);

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

}
