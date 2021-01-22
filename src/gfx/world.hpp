#pragma once

#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "volk/volk.h"
#include "base/types.hpp"
#include "base/math.hpp"
#include "sys/vk/buffer.hpp"
#include "gfx/context.hpp"
#include "gfx/base.hpp"
#include "gfx/mesh.hpp"

namespace minote::gfx {

using namespace base;

struct World {

	struct Uniforms {

		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 viewProjection;
		glm::vec4 lightPosition;
		glm::vec4 lightColor;
		glm::vec4 ambientColor;

		void setViewProjection(glm::uvec2 viewport, f32 fovy, f32 zNear, f32 zFar,
			glm::vec3 eye, glm::vec3 center, glm::vec3 up = {0.0f, 1.0f, 0.0f}) {
			auto const rawview = glm::lookAt(eye, center, up);
			auto const yFlip = base::make_scale(glm::vec3{1.0f, -1.0f, 1.0f});
			projection = glm::perspective(fovy,
				static_cast<f32>(viewport.x) / static_cast<f32>(viewport.y), zNear, zFar);
			view = yFlip * rawview;
			viewProjection = projection * view;
		}

	} uniforms;

	void create(Context& ctx, MeshBuffer& meshes);

	void destroy(Context& ctx);

	auto getDescriptorSetLayout() { return m_worldDescriptorSetLayout; }

	auto getDescriptorSet(i64 frameIndex) -> VkDescriptorSet& { return m_worldDescriptorSet[frameIndex]; }

	auto getDescriptorSets() { return m_worldDescriptorSet; }

	void uploadUniforms(Context& ctx, i64 frameIndex);

	void setDebugName(Context& ctx);

private:

	VkDescriptorSetLayout m_worldDescriptorSetLayout;
	PerFrame<sys::vk::Buffer> m_worldUniforms;
	PerFrame<VkDescriptorSet> m_worldDescriptorSet;

};

}
