#pragma once

#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

struct World {

	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection;
	glm::vec4 lightPosition;
	glm::vec4 lightColor;
	glm::vec4 ambientColor;

	void setViewProjection(glm::uvec2 viewport, base::f32 fovy, base::f32 zNear, base::f32 zFar,
		glm::vec3 eye, glm::vec3 center, glm::vec3 up = {0.0f, 1.0f, 0.0f}) {
		auto const rawview = glm::lookAt(eye, center, up);
		auto const yFlip = base::make_scale(glm::vec3{1.0f, -1.0f, 1.0f});
		projection = glm::perspective(fovy,
			static_cast<base::f32>(viewport.x) / static_cast<base::f32>(viewport.y),
			zNear, zFar);
		view = yFlip * rawview;
		viewProjection = projection * view;
	}

};

}
