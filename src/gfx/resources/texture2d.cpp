#include "gfx/resources/texture2d.hpp"

namespace minote::gfx {

auto Texture2D::mipView(u32 _mip) -> vuk::Unique<vuk::ImageView> {
	
	return handle->view.mip_subrange(_mip, 1).apply();
	
}

auto Texture2D::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eImage, _access);
	
}

void Texture2D::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(*handle), _initial, _final);
	
}

}
