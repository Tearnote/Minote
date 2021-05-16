#include "gfx/ibl.hpp"

#include "vuk/CommandBuffer.hpp"
#include "gfx/iblCoeffs.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

IBLMap::IBLMap(vuk::Context& ctx, vuk::PerThreadContext& ptc) {
	mapUnfiltered = ctx.allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = Format,
		.extent = {BaseSize, BaseSize, 1},
		.mipLevels = MipCount,
		.arrayLayers = 6,
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled | vuk::ImageUsageFlagBits::eTransferSrc,
	});
	mapUnfiltered.view = ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *mapUnfiltered.image,
		.viewType = vuk::ImageViewType::eCube,
		.format = mapUnfiltered.format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6,
		},
	});

	mapFiltered = ctx.allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = Format,
		.extent = {BaseSize, BaseSize, 1},
		.mipLevels = MipCount,
		.arrayLayers = 6,
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled | vuk::ImageUsageFlagBits::eTransferDst,
	});
	mapFiltered.view = ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *mapFiltered.image,
		.viewType = vuk::ImageViewType::eCube,
		.format = mapFiltered.format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6,
		},
	});

	for (auto i = 0u; i < arrayViewsUnfiltered.size(); i += 1) {
		arrayViewsUnfiltered[i] = ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *mapUnfiltered.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = mapUnfiltered.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6,
			},
		});
	}

	for (auto i = 0u; i < arrayViewsFiltered.size(); i += 1) {
		arrayViewsFiltered[i] = ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *mapFiltered.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = mapFiltered.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6,
			},
		});
	}

	auto iblPrefilterPci = vuk::ComputePipelineCreateInfo();
	iblPrefilterPci.add_spirv(std::vector<u32>{
#include "spv/iblPrefilter.comp.spv"
	}, "iblPrefilter.comp");
	ctx.create_named_pipeline("ibl_prefilter", iblPrefilterPci);

	auto iblPostfilterPci = vuk::ComputePipelineCreateInfo();
	iblPostfilterPci.add_spirv(std::vector<u32>{
#include "spv/iblPostfilter.comp.spv"
	}, "iblPostfilter.comp");
	ctx.create_named_pipeline("ibl_postfilter", iblPostfilterPci);
}

auto IBLMap::filter() -> vuk::RenderGraph {
	auto rg = vuk::RenderGraph();
	rg.add_pass({
		.name = "IBL prefilter",
		.resources = {
			"ibl_map_unfiltered"_image(vuk::eComputeRW),
		},
		.execute = [this](vuk::CommandBuffer& cmd) {
			for (auto i = 1u; i < MipCount; i += 1) {
				if (i != 1)
					cmd.image_barrier("ibl_map_unfiltered", vuk::eComputeWrite, vuk::eComputeRead);

				cmd.bind_sampled_image(0, 0, "ibl_map_unfiltered", vuk::SamplerCreateInfo{
					.magFilter = vuk::Filter::eLinear,
					.minFilter = vuk::Filter::eLinear,
					.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
					.addressModeV = vuk::SamplerAddressMode::eClampToEdge})
				   .bind_storage_image(0, 1, *arrayViewsUnfiltered[i])
				   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, float(i - 1))
				   .bind_compute_pipeline("ibl_prefilter");
				cmd.dispatch_invocations(BaseSize >> i, BaseSize >> i, 6);
			}
		},
	});
	rg.add_pass({
		.name = "IBL postfilter",
		.resources = {
			"ibl_map_unfiltered"_image(vuk::eComputeRead),
			"ibl_map_filtered"_image(vuk::eComputeWrite),
		},
		.execute = [this](vuk::CommandBuffer& cmd) {
			cmd.bind_sampled_image(0, 0, "ibl_map_unfiltered", vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.mipmapMode = vuk::SamplerMipmapMode::eLinear,
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_storage_image(0, 1, *arrayViewsFiltered[1])
			   .bind_storage_image(0, 2, *arrayViewsFiltered[2])
			   .bind_storage_image(0, 3, *arrayViewsFiltered[3])
			   .bind_storage_image(0, 4, *arrayViewsFiltered[4])
			   .bind_storage_image(0, 5, *arrayViewsFiltered[5])
			   .bind_storage_image(0, 6, *arrayViewsFiltered[6])
			   .bind_storage_image(0, 7, *arrayViewsFiltered[7])
			   .bind_compute_pipeline("ibl_postfilter");
			auto* coeffs = cmd.map_scratch_uniform_binding<vec4[7][5][3][24]>(0, 8);
			std::memcpy(coeffs, IBLCoefficients, sizeof(IBLCoefficients));
			cmd.dispatch_invocations(21840, 6);
		},
	});
	rg.add_pass({
		.name = "IBL mip 0 copy",
		.resources = {
			"ibl_map_unfiltered"_image(vuk::eTransferSrc),
			"ibl_map_filtered"_image(vuk::eTransferDst),
		},
		.execute = [this](vuk::CommandBuffer& cmd) {
			cmd.image_barrier("ibl_map_unfiltered", vuk::eComputeRead, vuk::eTransferSrc);
			cmd.image_barrier("ibl_map_filtered", vuk::eComputeWrite, vuk::eTransferDst);
			cmd.blit_image("ibl_map_unfiltered", "ibl_map_filtered", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{
					.aspectMask = vuk::ImageAspectFlagBits::eColor,
					.layerCount = 6,
				},
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{BaseSize, BaseSize, 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{
					.aspectMask = vuk::ImageAspectFlagBits::eColor,
					.layerCount = 6,
				},
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{BaseSize, BaseSize, 1}},
			}, vuk::Filter::eNearest);
		},
	});
	rg.attach_image("ibl_map_unfiltered", vuk::ImageAttachment::from_texture(mapUnfiltered), {}, {});
	rg.attach_image("ibl_map_filtered", vuk::ImageAttachment::from_texture(mapFiltered), {}, {});
	return rg;
}

}
