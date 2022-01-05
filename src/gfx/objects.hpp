#pragma once

#include <type_traits>
#include "base/containers/vector.hpp"
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
		
		bool visible; // Invisible objects are temporarily excluded from drawing
		bool exists; // Do not modify - internal garbage collection
		
		// Construct with default values
		static constexpr auto make_default() -> Metadata { return Metadata{true, true}; }
		
	};
	
	// Spatial properties
	struct Transform {
		
		vec3 position;
		f32 pad0;
		vec3 scale;
		f32 pad1;
		quat rotation;
		
		// Construct with default values
		static constexpr auto make_default() -> Transform {
			
			return Transform{
				.position = vec3(0.0f),
				.scale = vec3(1.0f),
				.rotation = quat::identity()};
			
		}
		
	};
	
	// Convenient access to all properties of a single object
	struct Proxy {
		
		Metadata& metadata;
		ID& modelID;
		vec4& color;
		Transform& transform;
		
	};
	
	// Ensure fast operation in large containers
	static_assert(std::is_trivially_constructible_v<Metadata>);
	static_assert(std::is_trivially_constructible_v<Transform>);
	
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
	
	void copyTransforms();
	
	// Return the current size of the pool. This might include nonexistent
	// objects, so it's only useful for transfers.
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
	// Direct access to these is discouraged, unless you're doing a whole
	// container transfer.
	ivector<Metadata> metadata;
	ivector<ID> modelIDs;
	ivector<vec4> colors;
	ivector<Transform> transforms;
	ivector<Transform> prevTransforms;
	
private:
	
	ivector<ObjectID> m_deletedIDs;
	
};

using ObjectID = ObjectPool::ObjectID;

}
