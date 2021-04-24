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

		glm::mat4 transform;
		glm::vec4 tint;
		f32 roughness;
		f32 metalness;
		u32 meshID; // internal
		f32 pad0;

	};
	hashmap<ID, std::vector<Instance>> instances;

	void addInstances(ID mesh, std::span<Instance const> instances);

	[[nodiscard]]
	auto size() const { return instances.size(); }
	void clear() { instances.clear(); }

};

}
