#pragma once

#include <vector>
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"

namespace minote::gfx {

using namespace base;

struct Objects {
	
	using ObjectID = usize;
	
	struct Metadata {
		
		bool visible = true;
		bool exists = true;
		
	};
	
	struct Material {
		
		vec4 tint = vec4(1.0f);
		f32 roughness;
		f32 metalness;
		
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
	std::vector<ID> meshIDs;
	std::vector<mat4> transforms;
	std::vector<mat4> prevTransforms;
	std::vector<Material> materials;
	
	[[nodiscard]]
	auto create() -> ObjectID;
	[[nodiscard]]
	auto createStatic(Object const&) -> ObjectID;
	[[nodiscard]]
	auto createDynamic(Object const&) -> Object;
	
	void destroy(ObjectID);
	void destroy(Object const&);
	
	void update(Object const&);
	
	void updatePrevTransforms();
	
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
private:
	
	std::vector<ObjectID> deletedIDs;
	
};

using ObjectID = Objects::ObjectID;
using Object = Objects::Object;

}
