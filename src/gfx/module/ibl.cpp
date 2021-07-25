#include "gfx/module/ibl.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "gfx/module/iblCoeffs.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;

IBLMap::IBLMap(vuk::PerThreadContext& _ptc) {
	
	mapUnfiltered = _ptc.ctx.allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = Format,
		.extent = {BaseSize, BaseSize, 1},
		.mipLevels = MipCount,
		.arrayLayers = 6,
		.usage =
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eTransferSrc });
	mapUnfiltered.view = _ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *mapUnfiltered.image,
		.viewType = vuk::ImageViewType::eCube,
		.format = mapUnfiltered.format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6 }});
	
	mapFiltered = _ptc.ctx.allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = Format,
		.extent = {BaseSize, BaseSize, 1},
		.mipLevels = MipCount,
		.arrayLayers = 6,
		.usage =
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eTransferDst });
	mapFiltered.view = _ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *mapFiltered.image,
		.viewType = vuk::ImageViewType::eCube,
		.format = mapFiltered.format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6 }});
	
	for (auto i: iota(0u, arrayViewsUnfiltered.size())) {
		
		arrayViewsUnfiltered[i] = _ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *mapUnfiltered.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = mapUnfiltered.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6 }});
		
	}
	
	for (auto i: iota(0u, arrayViewsFiltered.size())) {
		
		arrayViewsFiltered[i] = _ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *mapFiltered.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = mapFiltered.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6 }});
		
	}
	
	if (!pipelinesCreated) {
		
		auto iblPrefilterPci = vuk::ComputePipelineCreateInfo();
		iblPrefilterPci.add_spirv(std::vector<u32>{
#include "spv/iblPrefilter.comp.spv"
		}, "iblPrefilter.comp");
		_ptc.ctx.create_named_pipeline("ibl_prefilter", iblPrefilterPci);
		
		auto iblPostfilterPci = vuk::ComputePipelineCreateInfo();
		iblPostfilterPci.add_spirv(std::vector<u32>{
#include "spv/iblPostfilter.comp.spv"
		}, "iblPostfilter.comp");
		_ptc.ctx.create_named_pipeline("ibl_postfilter", iblPostfilterPci);
		
		pipelinesCreated = true;
		
	}
	
}

auto IBLMap::filter() -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "IBL prefilter",
		.resources = {
			vuk::Resource(Unfiltered_n, vuk::Resource::Type::eImage, vuk::eComputeRW) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(1u, MipCount)) {
				
				if (i != 1)
					cmd.image_barrier(Unfiltered_n, vuk::eComputeWrite, vuk::eComputeRead);
				
				cmd.bind_sampled_image(0, 0, Unfiltered_n, LinearClamp)
				   .bind_storage_image(0, 1, *arrayViewsUnfiltered[i])
				   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, float(i - 1))
				   .bind_compute_pipeline("ibl_prefilter");
				cmd.dispatch_invocations(BaseSize >> i, BaseSize >> i, 6);
				
			}
			
		}});
	
	rg.add_pass({
		.name = "IBL postfilter",
		.resources = {
			vuk::Resource(Unfiltered_n, vuk::Resource::Type::eImage, vuk::eComputeRead),
			vuk::Resource(Filtered_n,   vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, Unfiltered_n, TrilinearClamp)
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
			
		}});
	
	rg.add_pass({
		.name = "IBL mip 0 copy",
		.resources = {
			vuk::Resource(Unfiltered_n, vuk::Resource::Type::eImage, vuk::eTransferSrc),
			vuk::Resource(Filtered_n,   vuk::Resource::Type::eImage, vuk::eTransferDst) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.image_barrier(Unfiltered_n, vuk::eComputeRead,  vuk::eTransferSrc);
			cmd.image_barrier(Filtered_n,   vuk::eComputeWrite, vuk::eTransferDst);
			
			cmd.blit_image(Unfiltered_n, Filtered_n,
				vuk::ImageBlit{
					.srcSubresource = vuk::ImageSubresourceLayers{
						.aspectMask = vuk::ImageAspectFlagBits::eColor,
						.layerCount = 6 },
					.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{BaseSize, BaseSize, 1}},
					.dstSubresource = vuk::ImageSubresourceLayers{
						.aspectMask = vuk::ImageAspectFlagBits::eColor,
						.layerCount = 6 },
					.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{BaseSize, BaseSize, 1}} },
				vuk::Filter::eNearest);
			
		}});
	
	rg.attach_image(Unfiltered_n,
		vuk::ImageAttachment::from_texture(mapUnfiltered),
		vuk::eNone,
		vuk::eNone);
	rg.attach_image(Filtered_n,
		vuk::ImageAttachment::from_texture(mapFiltered),
		vuk::eNone,
		vuk::eNone);
	
	return rg;
	
}

}
