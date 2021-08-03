#pragma once

#include "base/container/vector.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/id.hpp"

namespace minote::gfx {

using namespace base;

// Pool of renderable objects.
struct ObjectPool {
	
	// Stable handle to an object in the pool
	using ObjectID = usize;
	
	// Object state
	struct Metadata {
		
		bool visible = true; // Invisible objects are temporarily excluded from drawing
		bool exists = true; // Do not modify - internal garbage collection
		
	};
	
	// Spatial properties
	struct Transform {
		
		vec3 position = vec3(0.0f);
		f32 pad0;
		vec3 scale = vec3(1.0f);
		f32 pad1;
		quat rotation = quat::identity();
		
	};
	
	// Shading properties
	struct Material {
		
		vec4 tint = vec4(1.0f); // Per-vertex albedo is multiplied by this
		f32 roughness; // 0.0f - glossy, 1.0f - rough
		f32 metalness; // 0.0f - dielectric, 1.0f - conductive
		vec2 pad0;
		
	};
	
	// Convenient access to all properties of a single object
	struct Proxy {
		
		Metadata& metadata;
		ID& meshID;
		Transform& transform;
		Material& material;
		
	};
	
	// Return a handle to a new, uninitialized object. See above which fields
	// do not have a default initializer. Remember to destroy() the object.
	[[nodiscard]]
	auto create() -> ObjectID;
	
	// Destroy the object, freeing up the ID for use with objects created
	// in the future.
	void destroy(ObjectID);
	
	// Return a proxy for convenient access to an object's properties. The proxy
	// can only be considered valid until any other ObjectPool method is called.
	[[nodiscard]]
	auto get(ObjectID) -> Proxy;
	
	// Return the current size of the pool. This might include nonexistent
	// objects, so it's only useful for transfers.
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
	// Direct access to these is discouraged, unless you're doing a whole
	// container transfer.
	ivector<Metadata> metadata;
	ivector<ID> meshIDs;
	ivector<Transform> transforms;
	ivector<Material> materials;
	
private:
	
	ivector<ObjectID> m_deletedIDs;
	
};

using ObjectID = ObjectPool::ObjectID;

}
