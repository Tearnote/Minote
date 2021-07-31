#include "gfx/objects.hpp"

#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto Objects::create() -> ObjectID {
	
	if (deletedIDs.empty()) {
		
		metadata.emplace_back();
		meshID.emplace_back();
		transform.emplace_back(mat4::identity());
		material.emplace_back();
		
		return size() - 1;
		
	} else {
		
		auto id = deletedIDs.back();
		deletedIDs.pop_back();
		metadata[id] = Metadata();
		transform[id] = mat4::identity();
		material[id] = Material();
		return id;
		
	}
	
}

void Objects::destroy(ObjectID _id) {
	
	metadata[_id].exists = false;
	deletedIDs.push_back(_id);
	
}

}
