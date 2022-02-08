#pragma once

#include "vuk/Image.hpp"

namespace minote::gfx {

// Commonly used sampler presets.

constexpr auto NearestClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eNearest,
	.minFilter = vuk::Filter::eNearest,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

constexpr auto LinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

constexpr auto TrilinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

constexpr auto TrilinearRepeat = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eRepeat,
	.addressModeV = vuk::SamplerAddressMode::eRepeat };

inline auto const MinClampRMCI = VkSamplerReductionModeCreateInfo{
	.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO,
	.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN };

inline auto const MinClamp = vuk::SamplerCreateInfo{
	.pNext = &MinClampRMCI,
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

}
