#include "gfx/objects.hpp"

#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto Objects::create() -> ObjectID {
	
	if (deletedIDs.empty()) {
		
		metadata.emplace_back();
		meshIndex.emplace_back();
		transform.emplace_back(mat4::identity());
		material.emplace_back();
		
		return size() - 1;
		
	} else {
		
		auto id = deletedIDs.back();
		deletedIDs.pop_back();
		metadata[id].exists = true;
		return id;
		
	}
	
}

auto Objects::createStatic(Object const& _object) -> ObjectID {
	
	auto objTransform =
		mat4::translate(_object.position) *
		mat4(_object.rotation) *
		mat4::scale(_object.scale);
	
	auto id = create();
	metadata[id].visible   = _object.visible;
	meshIndex[id]          = meshes.descriptorIDs.at(_object.mesh);
	transform[id]          = objTransform;
	material[id].tint      = _object.tint;
	material[id].roughness = _object.roughness;
	material[id].metalness = _object.metalness;
	
	return id;
	
}

auto Objects::createDynamic(Object const& _object) -> Object {
	
	auto result = _object;
	result.id = create();
	return result;
	
}

void Objects::destroy(ObjectID _id) {
	
	metadata[_id].exists = false;
	deletedIDs.push_back(_id);
	
}

void Objects::destroy(Object const& _object) {
	
	destroy(_object.id);
	
}

void Objects::update(Object const& _object) {
	
	auto objTransform =
		mat4::translate(_object.position) *
		mat4(_object.rotation) *
		mat4::scale(_object.scale);
	
	metadata[_object.id].visible   = _object.visible;
	meshIndex[_object.id]          = meshes.descriptorIDs.at(_object.mesh);
	transform[_object.id]          = objTransform;
	material[_object.id].tint      = _object.tint;
	material[_object.id].roughness = _object.roughness;
	material[_object.id].metalness = _object.metalness;
	
	
}

}
