#include "gfx/materials.hpp"

#include <cstring>

namespace minote::gfx {

template<typename T>
void MaterialList::addMaterial(ID _name, T _material) {
	
	m_materialIDs.emplace(_name, m_materials.size());
	
	auto& material = m_materials.emplace_back();
	std::memcpy(&material, &_material, sizeof(T));
	
}

}
