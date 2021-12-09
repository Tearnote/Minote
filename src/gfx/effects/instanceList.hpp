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

struct BasicInstanceList {
	
	using BasicTransform = ObjectPool::Transform;
	
	struct Instance {
		u32 meshIdx;
		u32 transformIdx;
	};
	
	static constexpr auto MaxInstances = 262144_zu;
	
	Buffer<uvec4> instancesCount;
	Buffer<Instance> instances;
	Buffer<vec4> colors;
	Buffer<BasicTransform> basicTransforms;
	
	static auto upload(Pool&, Frame&, vuk::Name, ObjectPool const&) -> BasicInstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return MaxInstances; }
	
};

struct InstanceList {
	
	using Instance = BasicInstanceList::Instance;
	using Transform = array<vec4, 3>;
	
	Buffer<uvec4> instancesCount;
	Buffer<Instance> instances;
	Buffer<vec4> colors;
	Buffer<Transform> transforms;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromBasic(Pool&, Frame&, vuk::Name, BasicInstanceList&&) -> InstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return BasicInstanceList::MaxInstances; }
	
};

struct DrawableInstanceList {
	
	using Command = VkDrawIndexedIndirectCommand;
	using Instance = BasicInstanceList::Instance;
	using Transform = array<vec4, 3>;
	
	Buffer<Command> commands;
	Buffer<uvec4> instancesCount;
	Buffer<Instance> instances;
	Buffer<vec4> colors;
	Buffer<Transform> transforms;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromUnsorted(Pool&, Frame&, vuk::Name, InstanceList) -> DrawableInstanceList;
	
	static auto frustumCull(Pool&, Frame&, vuk::Name, DrawableInstanceList,
		mat4 view, mat4 projection) -> DrawableInstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return BasicInstanceList::MaxInstances; }
	
};

}
