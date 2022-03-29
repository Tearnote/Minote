#include "gfx/resources/texture2dms.hpp"

#include "gfx/util.hpp"

namespace minote::gfx {

auto Texture2DMS::make(Pool& _pool, vuk::Name _name, uvec2 _size, vuk::Format _format,
	vuk::ImageUsageFlags _usage, vuk::SampleCountFlagBits _samples) -> Texture2DMS {
	
	auto& texture = [&_pool, _name, _format, _size, _usage, _samples]() -> vuk::Texture& {
		
		if (_pool.contains(_name)) {
			
			return _pool.get<vuk::Texture>(_name);
			
		} else {
			
			return _pool.insert<vuk::Texture>(_name,
				_pool.ptc().allocate_texture(vuk::ImageCreateInfo{
					.format = _format,
					.extent = {_size.x(), _size.y(), 1},
					.samples = _samples,
					.usage = _usage }));
			
		}
		
	}();
	
	_pool.ptc().ctx.debug.set_name(*texture.image, _name);
	_pool.ptc().ctx.debug.set_name(texture.view->payload, nameAppend(_name, "main"));
	
	return Texture2DMS{
		.name = _name,
		.handle = &texture };
	
}

auto Texture2DMS::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eImage, _access);
	
}

void Texture2DMS::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final,
	vuk::Clear _clear) const {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(*handle, _clear), _initial, _final);
	
}

}
