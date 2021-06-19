#pragma once

#include <string_view>
#include <vector>
#include <span>
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "base/container/hashmap.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"
#include "memory.hpp"

namespace minote::gfx {

using namespace base;

struct Meshes {
	
	std::vector<vec3> vertices;
	std::vector<vec3> normals;
	std::vector<u16vec4> colors;
	std::vector<u16> indices;
	
	vuk::Unique<vuk::Buffer> verticesBuf;
	vuk::Unique<vuk::Buffer> normalsBuf;
	vuk::Unique<vuk::Buffer> colorsBuf;
	vuk::Unique<vuk::Buffer> indicesBuf;
	
	struct Descriptor {
		
		u32 indexOffset;
		u32 indexCount;
		u32 vertexOffset;
		f32 radius;
		vec3 aabbMin;
		vec3 aabbMax;
	
	};
	std::vector<Descriptor> descriptors;
	hashmap<ID, size_t, Permanent> descriptorIDs;
	
	void addGltf(std::string_view name, std::span<char const> mesh);
	
	void upload(vuk::PerThreadContext&);
	
	[[nodiscard]]
	auto at(ID id) -> Descriptor& { return descriptors[descriptorIDs.at(id)]; }
	[[nodiscard]]
	auto at(ID id) const -> Descriptor const& { return descriptors[descriptorIDs.at(id)]; }
	
	auto size() const { return descriptors.size(); }
	
};

}
