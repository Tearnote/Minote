#include "gfx/resources/texture2d.hpp"

namespace minote::gfx {

Texture2D::Texture2D(vuk::PerThreadContext& _ptc, vuk::Name _name, uvec2 _size,
	vuk::Format _format, vuk::ImageUsageFlags _usage, u32 _mips) {
	
	name = _name;
	
	texture = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = _format,
		.extent = {_size.x(), _size.y(), 1},
		.mipLevels = _mips,
		.usage = _usage });
	
}

auto Texture2D::mipView(u32 mip) -> vuk::Unique<vuk::ImageView> {
	
	return texture.view.mip_subrange(mip, 1).apply();
	
}

auto Texture2D::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eImage, _access);
	
}

void Texture2D::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(texture), _initial, _final);
	
}

}
