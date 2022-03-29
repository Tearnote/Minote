#include "gfx/resources/texture2d.hpp"

#include "gfx/util.hpp"

namespace minote::gfx {

auto Texture2D::make(Pool& _pool, vuk::Name _name, uvec2 _size, vuk::Format _format,
	vuk::ImageUsageFlags _usage, u32 _mips) -> Texture2D {
	
	auto& texture = [&_pool, _name, _format, _size, _mips, _usage]() -> vuk::Texture& {
		
		if (_pool.contains(_name)) {
			
			return _pool.get<vuk::Texture>(_name);
			
		} else {
			
			return _pool.insert<vuk::Texture>(_name,
				_pool.ptc().allocate_texture(vuk::ImageCreateInfo{
					.format = _format,
					.extent = {_size.x(), _size.y(), 1},
					.mipLevels = _mips,
					.usage = _usage }));
			
		}
		
	}();
	
	_pool.ptc().ctx.debug.set_name(*texture.image, _name);
	_pool.ptc().ctx.debug.set_name(texture.view->payload, nameAppend(_name, "main"));
	
	return Texture2D{
		.name = _name,
		.handle = &texture };
	
}

auto Texture2D::mipView(u32 _mip) const -> vuk::Unique<vuk::ImageView> {
	
	//TODO add debug name
	return handle->view.mip_subrange(_mip, 1).apply();
	
}

auto Texture2D::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eImage, _access);
	
}

void Texture2D::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final,
	vuk::Clear _clear) const {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(*handle, _clear), _initial, _final);
	
}

}
