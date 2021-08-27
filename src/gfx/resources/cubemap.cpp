#include "gfx/resources/cubemap.hpp"

#include <cassert>
#include "gfx/util.hpp"
#include "base/util.hpp"

namespace minote::gfx {

using namespace base;

Cubemap::Cubemap(vuk::PerThreadContext& _ptc, vuk::Name _name, u32 _size,
	vuk::Format _format, vuk::ImageUsageFlags _usage) {
	
	name = _name;
	arrayViews.reserve(mipmapCount(_size));
	
	texture = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = _format,
		.extent = {_size, _size, 1},
		.mipLevels = mipmapCount(_size),
		.arrayLayers = 6,
		.usage = _usage });
	
	texture.view = _ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *texture.image,
		.viewType = vuk::ImageViewType::eCube,
		.format = texture.format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6 }});
	
	for (auto i: iota(0u, mipmapCount(_size))) {
		
		arrayViews.push_back(_ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *texture.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = texture.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6 }}));
		
	}
	
}

void Cubemap::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) {
	
	_rg.attach_image(name, vuk::ImageAttachment::from_texture(texture), _initial, _final);
	
}

}
