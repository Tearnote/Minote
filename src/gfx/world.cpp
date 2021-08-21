#include "gfx/world.hpp"

namespace minote::gfx {

auto World::upload(vuk::PerThreadContext& _ptc, vuk::Name _name) const -> Buffer<World> {
	
	return Buffer(_ptc, _name, *this, vuk::BufferUsageFlagBits::eUniformBuffer);
	
}

}
