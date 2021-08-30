#include "gfx/world.hpp"

namespace minote::gfx {

auto World::upload(ResourcePool& _pool, vuk::Name _name) const -> Buffer<World> {
	
	return _pool.make_buffer<World>(_name,
		vuk::BufferUsageFlagBits::eUniformBuffer, std::span(this, 1));
	
}

}
