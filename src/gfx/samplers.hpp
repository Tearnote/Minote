#pragma once

#include "vuk/Image.hpp"

namespace minote::gfx {

constexpr auto LinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge};

constexpr auto TrilinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge};

}
