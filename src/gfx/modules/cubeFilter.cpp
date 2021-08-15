#include "gfx/modules/cubeFilter.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/containers/string.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "gfx/modules/cubeFilterCoeffs.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;

void CubeFilter::compile(vuk::PerThreadContext& _ptc) {
	
	auto cubePrefilterPci = vuk::ComputePipelineCreateInfo();
	cubePrefilterPci.add_spirv(std::vector<u32>{
#include "spv/cubePrefilter.comp.spv"
	}, "cubePrefilter.comp");
	_ptc.ctx.create_named_pipeline("cube_prefilter", cubePrefilterPci);
	
	auto cubePostfilterPci = vuk::ComputePipelineCreateInfo();
	cubePostfilterPci.add_spirv(std::vector<u32>{
#include "spv/cubePostfilter.comp.spv"
	}, "cubePostfilter.comp");
	_ptc.ctx.create_named_pipeline("cube_postfilter", cubePostfilterPci);
	
}

auto CubeFilter::apply(string_view _name, Cubemap& _src, Cubemap& _dst) -> vuk::RenderGraph {
	
	assert(_src.texture.extent.width == BaseSize);
	assert(_src.texture.extent.height == BaseSize);
	assert(_dst.texture.extent.width == BaseSize);
	assert(_dst.texture.extent.height == BaseSize);
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = vuk::Name(sstring(_name) + " prefilt"),
		.resources = {
			vuk::Resource(_src.name, vuk::Resource::Type::eImage, vuk::eComputeRW) },
		.execute = [&](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(1u, MipCount)) {
				
				if (i != 1)
					cmd.image_barrier(_src.name, vuk::eComputeWrite, vuk::eComputeRead);
				
				cmd.bind_sampled_image(0, 0, _src.name, LinearClamp)
				   .bind_storage_image(0, 1, *_src.arrayViews[i])
				   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, float(i - 1))
				   .bind_compute_pipeline("cube_prefilter");
				cmd.dispatch_invocations(BaseSize >> i, BaseSize >> i, 6);
				
			}
			
		}});
	
	rg.add_pass({
		.name = vuk::Name(sstring(_name) + " postfilt"),
		.resources = {
			vuk::Resource(_src.name, vuk::Resource::Type::eImage, vuk::eComputeRead),
			vuk::Resource(_dst.name, vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [&](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _src.name, TrilinearClamp)
			   .bind_storage_image(0, 1, *_dst.arrayViews[1])
			   .bind_storage_image(0, 2, *_dst.arrayViews[2])
			   .bind_storage_image(0, 3, *_dst.arrayViews[3])
			   .bind_storage_image(0, 4, *_dst.arrayViews[4])
			   .bind_storage_image(0, 5, *_dst.arrayViews[5])
			   .bind_storage_image(0, 6, *_dst.arrayViews[6])
			   .bind_storage_image(0, 7, *_dst.arrayViews[7])
			   .bind_compute_pipeline("cube_postfilter");
			
			auto* coeffs = cmd.map_scratch_uniform_binding<vec4[7][5][3][24]>(0, 8);
			std::memcpy(coeffs, IBLCoefficients, sizeof(IBLCoefficients));
			
			cmd.dispatch_invocations(21840, 6);
			
		}});
	
	rg.add_pass({
		.name = vuk::Name(sstring(_name) + " mip0 copy"),
		.resources = {
			vuk::Resource(_src.name, vuk::Resource::Type::eImage, vuk::eTransferSrc),
			vuk::Resource(_dst.name, vuk::Resource::Type::eImage, vuk::eTransferDst) },
		.execute = [&](vuk::CommandBuffer& cmd) {
			
			cmd.image_barrier(_src.name, vuk::eComputeRead,  vuk::eTransferSrc);
			cmd.image_barrier(_dst.name, vuk::eComputeWrite, vuk::eTransferDst);
			
			cmd.blit_image(_src.name, _dst.name,
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
	
	return rg;
	
}

}
