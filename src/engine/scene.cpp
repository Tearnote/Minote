/**
 * Implementation of engine/scene.hpp
 * @file
 */

#include "engine/scene.hpp"

namespace minote {

void Scene::updateMatrices(uvec2 const size)
{
	vec2 const fsize = size;
	f32 const aspect = fsize.x / fsize.y;

	view = lookAt(camera.position, camera.target, camera.up);
	projection = perspective(radians(45.0f), aspect,
		camera.nearPlane, camera.farPlane);
	projection2D = ortho(0.0f, fsize.x, fsize.y, 0.0f, 1.0f, -1.0f);
}

}
