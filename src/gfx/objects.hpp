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
		vec2 pad0;
		
	};
	
	std::vector<Metadata> metadata;
	std::vector<ID> meshID;
	std::vector<mat4> transform;
	std::vector<Material> material;
	
	[[nodiscard]]
	auto create() -> ObjectID;
	
	void destroy(ObjectID);
	
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
private:
	
	std::vector<ObjectID> deletedIDs;
	
};

using ObjectID = Objects::ObjectID;

}
