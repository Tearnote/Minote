#include "gfx/world.hpp"

namespace minote::gfx {

auto World::upload(vuk::PerThreadContext& _ptc) -> vuk::Buffer {
	
	auto result = _ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(World), alignof(World));
	std::memcpy(result.mapped_ptr, this, sizeof(World));
	return result;
	
}

}
