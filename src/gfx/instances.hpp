#pragma once

#include <utility>
#include <vector>
#include <span>
#include "vuk/CommandBuffer.hpp"
#include "base/hashmap.hpp"
#include "base/types.hpp"
#include "base/id.hpp"
#include "gfx/meshes.hpp"

namespace minote::gfx {

using namespace base;

struct Instances {

	struct Instance {

		mat4 transform;
		vec4 tint;
		f32 roughness;
		f32 metalness;
		u32 meshID; // internal
		f32 pad0;

	};
	hashmap<ID, std::vector<Instance>> instances;

	void addInstances(ID mesh, std::span<Instance const> instances);

	[[nodiscard]]
	auto size() const { return instances.size(); }
	void clear();

};

}
