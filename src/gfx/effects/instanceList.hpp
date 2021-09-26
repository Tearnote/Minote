#pragma once

#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct BasicInstanceList {
	
	using MeshIndex = u32;
	using BasicTransform = ObjectPool::Transform;
	using Material = ObjectPool::Material;
	
	static constexpr auto MaxInstances = 262144_zu;
	
	Buffer<u32> instancesCount;
	Buffer<MeshIndex> meshIndices;
	Buffer<BasicTransform> basicTransforms;
	Buffer<Material> materials;
	
	static auto upload(Pool&, vuk::RenderGraph&, vuk::Name, ObjectPool const&, MeshBuffer const&) -> BasicInstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return MaxInstances; }
	
};

struct InstanceList {
	
	using MeshIndex = u32;
	using Transform = array<vec4, 3>;
	using Material = ObjectPool::Material;
	
	Buffer<u32> instancesCount;
	Buffer<MeshIndex> meshIndices;
	Buffer<Transform> transforms;
	Buffer<Material> materials;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromBasic(Pool&, vuk::RenderGraph&, vuk::Name, BasicInstanceList&&) -> InstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return BasicInstanceList::MaxInstances; }
	
};

struct DrawableInstanceList {
	
	using Command = VkDrawIndexedIndirectCommand;
	using MeshIndex = u32;
	using Transform = array<vec4, 3>;
	using Material = ObjectPool::Material;
	
	Buffer<Command> commands;
	Buffer<u32> instancesCount;
	Buffer<MeshIndex> meshIndices;
	Buffer<Transform> transforms;
	Buffer<Material> materials;
	
	static void compile(vuk::PerThreadContext&);
	
	static auto fromUnsorted(Pool&, vuk::RenderGraph&, vuk::Name, InstanceList const&,
		MeshBuffer const&) -> DrawableInstanceList;
	
	static auto frustumCull(Pool&, vuk::RenderGraph&, vuk::Name, DrawableInstanceList const&,
		MeshBuffer const&, mat4 view, mat4 projection) -> DrawableInstanceList;
	
	[[nodiscard]]
	auto capacity() const -> usize { return BasicInstanceList::MaxInstances; }
	
};

}
