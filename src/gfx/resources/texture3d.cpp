#include "gfx/resources/texture3d.hpp"

#include "gfx/util.hpp"

namespace minote::gfx {

auto Texture3D::make(Pool& _pool, vuk::Name _name, uvec3 _size, vuk::Format _format,
	vuk::ImageUsageFlags _usage) -> Texture3D {
	
	auto& texture = [&_pool, _name, _format, _size, _usage]() -> vuk::Texture& {
		
		if (_pool.contains(_name)) {
			
			return _pool.get<vuk::Texture>(_name);
			
		} else {
			
			return _pool.insert<vuk::Texture>(_name,
				_pool.ptc().allocate_texture(vuk::ImageCreateInfo{
					.format = _format,
					.extent = {_size.x(), _size.y(), _size.z()},
					.usage = _usage }));
			
		}
		
	}();
	
	_pool.ptc().ctx.debug.set_name(*texture.image, _name);
	_pool.ptc().ctx.debug.set_name(texture.view->payload, nameAppend(_name, "main"));
	
	return Texture3D{
		.name = _name,
		.handle = &texture };
	
}

auto Texture3D::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eImage, _access);
	
}

void Texture3D::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final,
	vuk::Clear _clear) const {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(*handle, _clear), _initial, _final);
	
}

}
