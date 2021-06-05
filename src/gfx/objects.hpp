#pragma once

#include <vector>
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"

namespace minote::gfx {

using namespace base;

struct Objects {
	
	struct Metadata {
		
		bool visible = true;
		bool exists = true;
		
	};
	
	struct Material {
		
		vec4 tint = vec4(1.0f);
		f32 roughness;
		f32 metalness;
		
	};
	
	struct StaticProxy {
		
		bool visible = true;
		ID mesh;
		mat4 transform = mat4(1.0f);
		vec4 tint = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		f32 roughness;
		f32 metalness;
		
	};
	
	using ObjectID = usize;
	
	std::vector<Metadata> metadata;
	std::vector<ID> meshIDs;
	std::vector<mat4> transforms;
	std::vector<mat4> prevTransforms;
	std::vector<Material> materials;
	
	[[nodiscard]]
	auto create() -> ObjectID;
	auto createStatic(StaticProxy const&) -> ObjectID;
	
	void destroy(ObjectID);
	
	void updatePrevTransforms();
	
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
private:
	
	std::vector<ObjectID> deletedIDs;
	
};

using ObjectID = Objects::ObjectID;

struct DynamicObject {
	
	bool visible = true;
	ID mesh;
	
	vec3 position = {0.0f, 0.0f, 0.0f};
	vec3 scale = {1.0f, 1.0f, 1.0f};
	mat3 rotation = mat3(1.0f);
	
	vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
	f32 roughness;
	f32 metalness;
	
	static auto create(Objects&) -> DynamicObject;
	void destroy(Objects&);
	
	void update(Objects&);
	
private:
	
	ObjectID id;
	
};

}
