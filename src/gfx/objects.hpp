#pragma once

#include <vector>
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"
#include "gfx/meshes.hpp"

namespace minote::gfx {

using namespace base;

struct Objects {
	
	using ObjectID = usize;
	
	struct Metadata {
		
		u32 visible = true;
		u32 exists = true;
		
	};
	
	struct Material {
		
		vec4 tint = vec4(1.0f);
		f32 roughness;
		f32 metalness;
		vec2 pad;
		
	};
	
	struct Object {
		
		bool visible = true;
		ID mesh;
		
		vec3 position = {0.0f, 0.0f, 0.0f};
		vec3 scale = {1.0f, 1.0f, 1.0f};
		mat3 rotation = mat3::identity();
		
		vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
		f32 roughness;
		f32 metalness;
		
		ObjectID id; // internal
		
	};
	
	std::vector<Metadata> metadata;
	std::vector<u32> meshIndex;
	std::vector<mat4> transform;
	std::vector<Material> material;
	
	explicit Objects(Meshes const& _meshes): meshes(_meshes) {}
	
	[[nodiscard]]
	auto create() -> ObjectID;
	[[nodiscard]]
	auto createStatic(Object const&) -> ObjectID;
	[[nodiscard]]
	auto createDynamic(Object const&) -> Object;
	
	void destroy(ObjectID);
	void destroy(Object const&);
	
	void update(Object const&);
	
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
private:
	
	Meshes const& meshes;
	std::vector<ObjectID> deletedIDs;
	
};

using ObjectID = Objects::ObjectID;
using Object = Objects::Object;

}
