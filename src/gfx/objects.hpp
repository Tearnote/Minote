#pragma once

#include <type_traits>
#include "util/vector.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/id.hpp"

namespace minote {

// Pool of renderable objects
struct ObjectPool {
	
	// Stable handle to an object in the pool
	using ObjectID = usize;
	
	// Object state
	struct Metadata {
		bool visible; // Invisible objects are excluded from drawing
		bool exists; // Do not modify - internal garbage collection
		
		// Construct with default values
		static constexpr auto make_default() -> Metadata { return Metadata{true, true}; }
	};
	
	// Spatial properties
	struct Transform {
		float3 position;
		float _pad0;
		float3 scale;
		float _pad1;
		quat rotation;
		
		// Construct with default values
		static constexpr auto make_default() -> Transform {
			return Transform{
				.position = {0.0f, 0.0f, 0.0f},
				.scale = {1.0f, 1.0f, 1.0f},
				.rotation = quat::identity(),
			};
		}
	};
	
	// Convenient access to all properties of a single object
	struct Proxy {
		Metadata& metadata;
		ID& modelID;
		float4& color;
		Transform& transform;
	};
	
	// Ensure fast operation in large containers
	static_assert(std::is_trivially_constructible_v<Metadata>);
	static_assert(std::is_trivially_constructible_v<Transform>);
	
	// Return a handle to a new object. You need to set at least modelID
	[[nodiscard]]
	auto create() -> ObjectID;
	
	// Mark non-drawable and free up the object slot for reuse
	void destroy(ObjectID);
	
	// Return a proxy for convenient access to an object. The proxy
	// is only valid until any other ObjectPool access
	[[nodiscard]]
	auto get(ObjectID) -> Proxy;
	
	// Call at the end of the frame to copy transforms to prevTransforms
	void copyTransforms();
	
	// Current size of the pool. Includes nonexistent objects
	[[nodiscard]]
	auto size() const -> usize { return metadata.size(); }
	
	ivector<Metadata> metadata;
	ivector<ID> modelIDs;
	ivector<float4> colors;
	ivector<Transform> transforms;
	ivector<Transform> prevTransforms;
	
private:
	
	ivector<ObjectID> m_deletedIDs;
	
};

using ObjectID = ObjectPool::ObjectID;
using Transform = ObjectPool::Transform;

}
