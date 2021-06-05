#include "gfx/objects.hpp"

#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto Objects::create() -> ObjectID {
	
	if (deletedIDs.empty()) {
		
		metadata.emplace_back();
		meshIDs.emplace_back();
		transforms.emplace_back(1.0f);
		prevTransforms.emplace_back(1.0f);
		materials.emplace_back();
		
		return size() - 1;
		
	} else {
		
		auto id = deletedIDs.back();
		deletedIDs.pop_back();
		
		metadata[id] = Metadata();
		meshIDs[id] = ID();
		transforms[id] = mat4(1.0f);
		prevTransforms[id] = mat4(1.0f);
		materials[id] = Material();
		
		return id;
		
	}
	
}

auto Objects::create(Proxy const& _proxy) -> ObjectID {
	
	auto id = create();
	metadata[id].visible = _proxy.visible;
	meshIDs[id] = _proxy.mesh;
	transforms[id] = _proxy.transform;
	prevTransforms[id] = _proxy.transform;
	materials[id].tint = _proxy.tint;
	materials[id].roughness = _proxy.roughness;
	materials[id].metalness = _proxy.metalness;
	
	return id;
	
}

void Objects::updatePrevTransforms() {
	
	std::memcpy(prevTransforms.data(), transforms.data(), sizeof(mat4) * size());
	
}

}
