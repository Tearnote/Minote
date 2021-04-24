#pragma once

#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "gfx/instances.hpp"
#include "gfx/meshes.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

struct Indirect {

	using Instance = Instances::Instance;

	struct Command {

		u32 indexCount;
		u32 instanceCount;
		u32 firstIndex;
		i32 vertexOffset;
		u32 firstInstance;
		// =====
		f32 meshRadius;

	};

	vuk::Buffer commands;
	size_t commandsCount;
	vuk::Buffer instances;
	size_t instancesCount;

	static auto createBuffers(vuk::PerThreadContext& ptc, Meshes const& meshes, Instances const& instances) -> Indirect;

};

}
