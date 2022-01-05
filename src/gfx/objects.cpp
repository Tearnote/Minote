#include "gfx/objects.hpp"

#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto ObjectPool::create() -> ObjectID {
	
	if (m_deletedIDs.empty()) {
		
		metadata.emplace_back(Metadata::make_default());
		modelIDs.emplace_back();
		colors.emplace_back(1.0f); // Fully opaque
		transforms.emplace_back(Transform::make_default());
		prevTransforms.emplace_back(Transform::make_default());
		
		return size() - 1;
		
	} else {
		
		auto id = m_deletedIDs.back();
		m_deletedIDs.pop_back();
		metadata[id] = Metadata::make_default();
		colors[id] = vec4(1.0f); // Fully opaque
		transforms[id] = Transform::make_default();
		prevTransforms[id] = Transform::make_default();
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
		.modelID = modelIDs[_id],
		.color = colors[_id],
		.transform = transforms[_id] };
	
}

void ObjectPool::copyTransforms() {
	
	prevTransforms = transforms;
	
}

}
