#pragma once

#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/materials.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct BasicInstanceList {
	
	using BasicTransform = ObjectPool::Transform;
	
	struct Instance {
		u32 meshIdx;
		u32 materialIdx;
		u32 colorIdx;
		u32 transformIdx;
	};
	
	static constexpr auto MaxInstances = 262144_zu;
	
	Buffer<uvec4> instancesCount;
	Buffer<Instance> instances;
	Buffer<vec4> colors;
	Buffer<BasicTransform> basicTransforms;
	
	static auto upload(Pool&, vuk::RenderGraph&, vuk::Name, ObjectPool const&,
		MeshBuffer const&, MaterialBuffer const&) -> BasicInstanceList;
	
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
	
	static auto fromBasic(Pool&, vuk::RenderGraph&, vuk::Name, BasicInstanceList&&) -> InstanceList;
	
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
	
	static auto fromUnsorted(Pool&, vuk::RenderGraph&, vuk::Name, InstanceList const&,
		MeshBuffer const&) -> DrawableInstanceList;
	
	static auto frustumCull(Pool&, vuk::RenderGraph&, vuk::Name, DrawableInstanceList const&,
		MeshBuffer const&, mat4 view, mat4 projection) -> DrawableInstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return BasicInstanceList::MaxInstances; }
	
};

}
