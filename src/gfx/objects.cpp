#include "gfx/objects.hpp"

#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto ObjectPool::create() -> ObjectID {
	
	if (m_deletedIDs.empty()) {
		
		metadata.emplace_back();
		meshIDs.emplace_back();
		transforms.emplace_back();
		materials.emplace_back();
		
		return size() - 1;
		
	} else {
		
		auto id = m_deletedIDs.back();
		m_deletedIDs.pop_back();
		metadata[id] = Metadata();
		transforms[id] = Transform();
		materials[id] = Material();
		return id;
		
	}
	
}

void ObjectPool::destroy(ObjectID _id) {
	
	metadata[_id].exists = false;
	m_deletedIDs.push_back(_id);
	
}

auto ObjectPool::get(ObjectID _id) -> Proxy {
	
	return Proxy{
		.metadata = metadata[_id],
		.meshID = meshIDs[_id],
		.transform = transforms[_id],
		.material = materials[_id]};
	
}

}
