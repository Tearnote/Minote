#include "gfx/effects/cubeFilter.hpp"

#include <cassert>
#include "vuk/CommandBuffer.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "gfx/effects/cubeFilterCoeffs.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote {

void CubeFilter::compile(vuk::PerThreadContext& _ptc) {
	
	auto cubeFilterPrePci = vuk::ComputePipelineBaseCreateInfo();
	cubeFilterPrePci.add_spirv(std::vector<u32>{
#include "spv/cubeFilter/pre.comp.spv"
	}, "cubeFilter/pre.comp");
	_ptc.ctx.create_named_pipeline("cubeFilter/pre", cubeFilterPrePci);
	
	auto cubeFilterPostPci = vuk::ComputePipelineBaseCreateInfo();
	cubeFilterPostPci.add_spirv(std::vector<u32>{
#include "spv/cubeFilter/post.comp.spv"
	}, "cubeFilter/post.comp");
	_ptc.ctx.create_named_pipeline("cubeFilter/post", cubeFilterPostPci);
	
}

void CubeFilter::apply(Frame& _frame, Cubemap _src, Cubemap _dst) {
	
	assert(_src.size() == uvec2(BaseSize));
	assert(_dst.size() == uvec2(BaseSize));
	
	_frame.rg.add_pass({
		.name = nameAppend(_src.name, "cubeFilter/pre"),
		.resources = {
			_src.resource(vuk::eComputeWrite) },
		.execute = [_src](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(1u, MipCount)) {
				
				cmd.image_barrier(_src.name, vuk::eComputeWrite, vuk::eComputeSampled, i-1, 1);
				
				cmd.bind_sampled_image(0, 0, *_src.mipView(i-1), LinearClamp)
				   .bind_storage_image(0, 1, *_src.mipArrayView(i))
				   .bind_compute_pipeline("cubeFilter/pre");
				
				cmd.specialize_constants(0, _src.size().x() >> i);
				
				cmd.dispatch_invocations(BaseSize >> i, BaseSize >> i, 6);
				
			}
			
			cmd.image_barrier(_src.name, vuk::eComputeSampled, vuk::eComputeWrite, 0, MipCount-1);
			
		}});
	
	_frame.rg.add_pass({
		.name = nameAppend(_src.name, "cubeFilter/post"),
		.resources = {
			_src.resource(vuk::eComputeRead),
			_dst.resource(vuk::eComputeWrite) },
		.execute = [_src, _dst](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _src.name, TrilinearClamp)
			   .bind_storage_image(0, 1, *_dst.mipArrayView(1))
			   .bind_storage_image(0, 2, *_dst.mipArrayView(2))
			   .bind_storage_image(0, 3, *_dst.mipArrayView(3))
			   .bind_storage_image(0, 4, *_dst.mipArrayView(4))
			   .bind_storage_image(0, 5, *_dst.mipArrayView(5))
			   .bind_storage_image(0, 6, *_dst.mipArrayView(6))
			   .bind_storage_image(0, 7, *_dst.mipArrayView(7))
			   .bind_compute_pipeline("cubeFilter/post");
			
			auto* coeffs = cmd.map_scratch_uniform_binding<vec4[7][5][3][24]>(0, 8);
			std::memcpy(coeffs, IBLCoefficients, sizeof(IBLCoefficients));
			
			cmd.dispatch_invocations(21840, 6);
			
		}});
	
	_frame.rg.add_pass({
		.name = nameAppend(_src.name, "mip0 copy"),
		.resources = {
			_src.resource(vuk::eTransferSrc),
			_dst.resource(vuk::eTransferDst) },
		.execute = [_src, _dst](vuk::CommandBuffer& cmd) {
			
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
	
}

}
