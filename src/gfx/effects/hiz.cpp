#include "gfx/effects/hiz.hpp"

#include "vuk/CommandBuffer.hpp"
#include "util/types.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote {

void HiZ::compile(vuk::PerThreadContext& _ptc) {
	
	auto hizFirstPci = vuk::ComputePipelineBaseCreateInfo();
	hizFirstPci.add_spirv(std::vector<uint>{
#include "spv/hiz/first.comp.spv"
	}, "hiz/first.comp");
	_ptc.ctx.create_named_pipeline("hiz/first", hizFirstPci);
	
	auto hizMipPci = vuk::ComputePipelineBaseCreateInfo();
	hizMipPci.add_spirv(std::vector<uint>{
#include "spv/hiz/mip.comp.spv"
	}, "hiz/mip.comp");
	_ptc.ctx.create_named_pipeline("hiz/mip", hizMipPci);
	
}

auto HiZ::make(Pool& _pool, vuk::Name _name, Texture2DMS _depth) -> Texture2D {
	
	auto dim = max(nextPOT(_depth.size().x()), nextPOT(_depth.size().y()));
	auto size = uint2(dim);
	return Texture2D::make(_pool, _name,
		size, vuk::Format::eR32Sfloat,
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eTransferDst,
		mipmapCount(dim));
	
}

void HiZ::fill(Frame& _frame, Texture2D _hiz, Texture2DMS _depth) {
	
	auto mipCount = mipmapCount(_hiz.size().x());
	
	_frame.rg.add_pass({
		.name = nameAppend(_hiz.name, "hiz/first"),
		.resources = {
			_depth.resource(vuk::eComputeSampled),
			_hiz.resource(vuk::eComputeWrite) },
		.execute = [mipCount, _hiz, _depth](vuk::CommandBuffer& cmd) {
			
			auto mipsGenerated = 0;
			
			// Initial pass
			
			cmd.bind_sampled_image(0, 0, _depth, NearestClamp)
			   .bind_storage_image(0, 1, *_hiz.mipView(min(0u, mipCount - 1)))
			   .bind_storage_image(0, 2, *_hiz.mipView(min(1u, mipCount - 1)))
			   .bind_storage_image(0, 3, *_hiz.mipView(min(2u, mipCount - 1)))
			   .bind_storage_image(0, 4, *_hiz.mipView(min(3u, mipCount - 1)))
			   .bind_storage_image(0, 5, *_hiz.mipView(min(4u, mipCount - 1)))
			   .bind_storage_image(0, 6, *_hiz.mipView(min(5u, mipCount - 1)));
			
			cmd.specialize_constants(0, uintFromuint16(_depth.size()));
			cmd.specialize_constants(1, uintFromuint16(_hiz.size()));
			cmd.specialize_constants(2, min(mipCount, 6u));
			
			cmd.bind_compute_pipeline("hiz/first");
			cmd.dispatch_invocations(_depth.size().x(), _depth.size().y());
			
			mipsGenerated += 6;
			
			while (mipsGenerated < mipCount) {
				
				cmd.image_barrier(_hiz.name, vuk::eComputeRW, vuk::eComputeSampled, mipsGenerated - 1, 1);
				
				cmd.bind_sampled_image(0, 0, *_hiz.mipView(min(mipsGenerated - 1, mipCount - 1)), MinClamp)
				   .bind_storage_image(0, 1, *_hiz.mipView(min(mipsGenerated + 0, mipCount - 1)))
				   .bind_storage_image(0, 2, *_hiz.mipView(min(mipsGenerated + 1, mipCount - 1)))
				   .bind_storage_image(0, 3, *_hiz.mipView(min(mipsGenerated + 2, mipCount - 1)))
				   .bind_storage_image(0, 4, *_hiz.mipView(min(mipsGenerated + 3, mipCount - 1)))
				   .bind_storage_image(0, 5, *_hiz.mipView(min(mipsGenerated + 4, mipCount - 1)))
				   .bind_storage_image(0, 6, *_hiz.mipView(min(mipsGenerated + 5, mipCount - 1)))
				   .bind_storage_image(0, 7, *_hiz.mipView(min(mipsGenerated + 6, mipCount - 1)));
				
				cmd.specialize_constants(0, uintFromuint16(_hiz.size() >> (mipsGenerated - 1u)));
				cmd.specialize_constants(1, min(mipCount - mipsGenerated, 7u));
				
				cmd.bind_compute_pipeline("hiz/mip");
				cmd.dispatch_invocations(_depth.size().x() / 4, _depth.size().y() / 4);
				
				cmd.image_barrier(_hiz.name, vuk::eComputeSampled, vuk::eComputeRW, mipsGenerated - 1, 1);
				
				mipsGenerated += 7;
				
			}
			
		}});
	
}

}
