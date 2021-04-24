#pragma once

#include <utility>
#include <vector>
#include <span>
#include "vuk/CommandBuffer.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "base/hashmap.hpp"
#include "base/types.hpp"
#include "base/id.hpp"
#include "gfx/meshes.hpp"

namespace minote::gfx {

using namespace base;

struct Instances {

	struct Instance {

		glm::mat4 transform = glm::mat4(1.0f);
		glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
		f32 roughness = 0.6f;
		f32 metalness = 0.1f;
		u32 meshID; // internal
		f32 pad1;

	};
	hashmap<ID, std::vector<Instance>> instances;

	void addInstances(ID mesh, std::span<Instance const> instances);

	auto size() const { return instances.size(); }
	void clear() { instances.clear(); }

};

}
