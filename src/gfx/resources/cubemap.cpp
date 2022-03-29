#include "gfx/resources/cubemap.hpp"

#include <utility>
#include "gfx/util.hpp"

namespace minote::gfx {

auto Cubemap::make(Pool& _pool, vuk::Name _name, u32 _size, vuk::Format _format,
	vuk::ImageUsageFlags _usage) -> Cubemap {
	
	auto& texture = [&_pool, _name, _format, _size, _usage]() -> vuk::Texture& {
		
		if (_pool.contains(_name)) {
			
			return _pool.get<vuk::Texture>(_name);
			
		} else {
			
			auto result = _pool.ptc().allocate_texture(vuk::ImageCreateInfo{
				.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
				.format = _format,
				.extent = {_size, _size, 1},
				.mipLevels = mipmapCount(_size),
				.arrayLayers = 6,
				.usage = _usage });
			result.view = _pool.ptc().create_image_view(vuk::ImageViewCreateInfo{
				.image = *result.image,
				.viewType = vuk::ImageViewType::eCube,
				.format = result.format,
				.subresourceRange = vuk::ImageSubresourceRange{
					.aspectMask = vuk::ImageAspectFlagBits::eColor,
					.levelCount = VK_REMAINING_MIP_LEVELS,
					.layerCount = 6 }});
			
			return _pool.insert(_name, std::move(result));
			
		}
		
	}();
	
	return Cubemap{
		.name = _name,
		.handle = &texture };
	
}

auto Cubemap::mipView(u32 _mip) const -> vuk::Unique<vuk::ImageView> {
	
	return handle->view.mip_subrange(_mip, 1).apply();
	
}

auto Cubemap::mipArrayView(u32 _mip) const -> vuk::Unique<vuk::ImageView> {
	
	return handle->view.mip_subrange(_mip, 1).view_as(vuk::ImageViewType::e2DArray).apply();
	
}
auto Cubemap::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eImage, _access);
	
}

void Cubemap::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) const {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(*handle), _initial, _final);
	
}

}
