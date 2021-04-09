#pragma once

#include <utility>
#include <vector>
#include <span>
#include "vuk/CommandBuffer.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "base/hashmap.hpp"
#include "base/id.hpp"
#include "gfx/meshbuffer.hpp"

namespace minote::gfx {

using namespace base;

struct InstanceBuffer {

	struct Instance {

		glm::mat4 transform = glm::mat4(1.0f);
		glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
		float roughness = 0.6f;
		float metalness = 0.1f;
		float pad0;
		float pad1;

	};

	void addInstances(ID mesh, std::span<Instance const> instances);

	auto makeIndirect(MeshBuffer const& meshBuffer)
		-> std::pair<std::vector<vuk::DrawIndexedIndirectCommand>, std::vector<Instance>>;

private:

	hashmap<ID, std::vector<Instance>> instances;

};

}
