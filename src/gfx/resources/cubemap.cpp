#include "gfx/resources/cubemap.hpp"

namespace minote::gfx {

auto Cubemap::mipView(u32 _mip) -> vuk::Unique<vuk::ImageView> {
	
	return handle->view.mip_subrange(_mip, 1).view_as(vuk::ImageViewType::e2DArray).apply();
	
}

}
