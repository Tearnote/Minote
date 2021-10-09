#pragma once

#include <variant>
#include "vuk/Name.hpp"
#include "base/containers/hashmap.hpp"
#include "base/containers/vector.hpp"
#include "base/containers/string.hpp"
#include "base/types.hpp"
#include "base/id.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

using namespace base;

struct MaterialBuffer;

struct MaterialList {
	
	enum struct Type: u32 {
		None = 0, // Background - typically used for sky
		PBR = 1,
		Count
	};
	
	struct PBR {
		
		Type id;
		f32 roughness; // 0.0f - glossy, 1.0f - rough
		f32 metalness;
		f32 pad0;
		
		static constexpr auto make(PBR params) -> PBR { params.id = Type::PBR; return params; }
		
	};
	
	union Material {
		PBR pbr;
	};
	
	template<typename T>
	void addMaterial(ID name, T material);
	
	auto upload(Pool&, vuk::Name) && -> MaterialBuffer;
	
private:
	
	ivector<Material> m_materials;
	hashmap<ID, usize> m_materialIDs;
	
};

using Material = MaterialList::Material;
using MaterialType = MaterialList::Type;
using MaterialPBR = MaterialList::PBR;

struct MaterialBuffer {
	Buffer<Material> materials;
	hashmap<ID, usize> materialIDs;
};

}

#include "gfx/materials.tpp"
