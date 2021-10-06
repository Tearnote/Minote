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
	
	// Shading properties
	struct Material {
		
		enum struct Type: u32 {
			None = 0, // Background - typically used for sky
			PBR = 1,
			Count
		};
		
		vec3 tint; // Per-vertex albedo is multiplied by this
		Type id; // Type of material to shade with
		f32 roughness; // 0.0f - glossy, 1.0f - rough
		f32 metalness; // 0.0f - dielectric, 1.0f - conductive
		vec2 pad0;
		
		// Construct with default values
		static constexpr auto make_default() -> Material {
			
			return Material{
				.tint = vec3(1.0f)};
			
		}
		
	};
	
	// Convenient access to all properties of a single object
	struct Proxy {
		
		Metadata& metadata;
		ID& meshID;
		Transform& transform;
		Material& material;
		
	};
	
	// Ensure fast operation in large containers
	static_assert(std::is_trivially_constructible_v<Metadata>);
	static_assert(std::is_trivially_constructible_v<Transform>);
	static_assert(std::is_trivially_constructible_v<Material>);
	
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
using MaterialType = ObjectPool::Material::Type;

constexpr auto MaterialCount = usize(MaterialType::Count);

}
