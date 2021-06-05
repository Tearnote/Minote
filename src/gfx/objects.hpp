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
	
	struct Proxy {
		
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
	auto create(Proxy const&) -> ObjectID;
	
	void destroy(ObjectID);
	
	void updatePrevTransforms();
	
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
private:
	
	std::vector<ObjectID> deletedIDs;
	
};

using ObjectID = Objects::ObjectID;

}
