#include "gfx/materials.hpp"

namespace minote::gfx {

auto MaterialList::upload(Pool& _pool, vuk::Name _name) && -> MaterialBuffer {
	
	auto result = MaterialBuffer{
		.materials = Buffer<Material>::make(_pool, _name,
			vuk::BufferUsageFlagBits::eStorageBuffer,
			m_materials, vuk::MemoryUsage::eGPUonly),
		.materialIDs = std::move(m_materialIDs) };
	
	// Clean up in case this isn't a temporary
	
	m_materials.clear();
	m_materials.shrink_to_fit();
	
	return result;
	
}

}
