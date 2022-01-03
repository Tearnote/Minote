#pragma once

#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/frame.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct BasicObjectList {
	
	using BasicTransform = ObjectPool::Transform;
	
	static constexpr auto MaxObjects = 65536_zu;
	
	Buffer<uvec4> objectsCount;
	Buffer<u32> modelIndices;
	Buffer<vec4> colors;
	Buffer<BasicTransform> basicTransforms;
	
	static auto upload(Pool&, Frame&, vuk::Name, ObjectPool const&) -> BasicObjectList;
	
	[[nodiscard]]
	static constexpr auto capacity() -> usize { return MaxObjects; }
	
};

struct ObjectList {
	
	using Transform = array<vec4, 3>;
	
	Buffer<uvec4> objectsCount;
	Buffer<u32> modelIndices;
	Buffer<vec4> colors;
	Buffer<Transform> transforms;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromBasic(Pool&, Frame&, vuk::Name, BasicObjectList&&) -> ObjectList;
	
	[[nodiscard]]
	static constexpr auto capacity() -> usize { return BasicObjectList::MaxObjects; }
	
};

struct InstanceList {
	
	using Command = VkDrawIndexedIndirectCommand;
	using Transform = array<vec4, 3>;
	
	struct Instance {
		u32 objectIdx;
		u32 meshIdx;
	};
	
	static constexpr auto MaxInstances = 262144_zu;
	
	// Object data
	Buffer<vec4> colors;
	Buffer<Transform> transforms;
	
	// Instance data
	Buffer<uvec4> instancesCount;
	Buffer<Instance> instances;
	
	// Draw data
	Buffer<Command> commands;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromObjects(Pool&, Frame&, vuk::Name, ObjectList, mat4 view, mat4 projection) -> InstanceList;
	
	[[nodiscard]]
	static constexpr auto capacity() -> usize { return MaxInstances; }
	
};

}
