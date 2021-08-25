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
	m_size = uvec2(_size);
	m_format = _format;
	
	auto texture = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = _format,
		.extent = {_size, _size, 1},
		.mipLevels = mipmapCount(_size),
		.arrayLayers = 6,
		.usage = _usage });
		
	image = texture.image.release();
	view = _ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *texture.image,
		.viewType = vuk::ImageViewType::eCube,
		.format = texture.format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6 }})
		.release();
	
	for (auto i: iota(0u, mipmapCount(_size))) {
		
		arrayViews.push_back(_ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *texture.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = texture.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6 }})
			.release());
		
	}
	
}

void Cubemap::recycle(vuk::PerThreadContext& _ptc) {
	
	assert(image);
	
	_ptc.ctx.enqueue_destroy(image);
	_ptc.ctx.enqueue_destroy(view);
	for (auto iv: arrayViews)
		_ptc.ctx.enqueue_destroy(iv);
	
}

void Cubemap::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) {
	
	_rg.attach_image(name, vuk::ImageAttachment{
		.image = image,
		.image_view = view,
		.extent = vukExtent(m_size),
		.format = m_format },
		_initial, _final);
	
}

}
