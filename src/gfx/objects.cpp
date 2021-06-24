#include "gfx/objects.hpp"

#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto Objects::create() -> ObjectID {
	
	if (deletedIDs.empty()) {
		
		metadata.emplace_back();
		meshIDs.emplace_back();
		transforms.emplace_back(mat4::identity());
		prevTransforms.emplace_back(mat4::identity());
		materials.emplace_back();
		
		return size() - 1;
		
	} else {
		
		auto id = deletedIDs.back();
		deletedIDs.pop_back();
		
		metadata[id] = Metadata();
		meshIDs[id] = ID();
		transforms[id] = mat4::identity();
		prevTransforms[id] = mat4::identity();
		materials[id] = Material();
		
		return id;
		
	}
	
}

auto Objects::createStatic(Object const& _object) -> ObjectID {
	
	auto translation = mat4::translate(_object.position);
	auto rotation = mat4(_object.rotation);
	auto scale = mat4::scale(_object.scale);
	
	auto transform = translation * rotation * scale;
	// auto transform =
	// 	mat4::translate(_object.position) *
	// 	mat4(_object.rotation) *
	// 	mat4::scale(_object.scale);
	
	auto id = create();
	metadata[id].visible    = _object.visible;
	meshIDs[id]             = _object.mesh;
	transforms[id]          = transform;
	prevTransforms[id]      = transform;
	materials[id].tint      = _object.tint;
	materials[id].roughness = _object.roughness;
	materials[id].metalness = _object.metalness;
	
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

void Objects::updatePrevTransforms() {
	
	std::memcpy(prevTransforms.data(), transforms.data(), sizeof(mat4) * size());
	
}

void Objects::update(Object const& _object) {
	
	auto transform =
		mat4::translate(_object.position) *
		mat4(_object.rotation) *
		mat4::scale(_object.scale);
	
	metadata[_object.id].visible    = _object.visible;
	meshIDs[_object.id]             = _object.mesh;
	transforms[_object.id]          = transform;
	materials[_object.id].tint      = _object.tint;
	materials[_object.id].roughness = _object.roughness;
	materials[_object.id].metalness = _object.metalness;
	
	
}

}
