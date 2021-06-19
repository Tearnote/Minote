#include "gfx/module/bloom.hpp"

#include <cassert>
#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;

Bloom::Bloom(vuk::PerThreadContext& _ptc, vuk::Extent2D _size) {
	
	assert(_size.width  >= (1 << BloomPasses));
	assert(_size.height >= (1 << BloomPasses));
	
	size = _size;
	
	bloom = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = BloomFormat,
		.extent = {_size.width >> 1, _size.height >> 1, 1},
		.mipLevels = BloomPasses,
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	
	for (auto i = 0u; i < BloomPasses; i += 1) {
		
		bloomViews[i] = _ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *bloom.image,
			.format = bloom.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
			},
		});
		
	}
	
	if (!pipelinesCreated) {
		
		auto bloomDownPci = vuk::ComputePipelineCreateInfo();
		bloomDownPci.add_spirv(std::vector<u32>{
#include "spv/bloomDown.comp.spv"
		}, "bloomDown.comp");
		_ptc.ctx.create_named_pipeline("bloom_down", bloomDownPci);
		
		auto bloomDownKarisPci = vuk::ComputePipelineCreateInfo();
		bloomDownKarisPci.add_spirv(std::vector<u32>{
#include "spv/bloomDownKaris.comp.spv"
		}, "bloomDownKaris.comp");
		_ptc.ctx.create_named_pipeline("bloom_down_karis", bloomDownKarisPci);
		
		auto bloomUpPci = vuk::ComputePipelineCreateInfo();
		bloomUpPci.add_spirv(std::vector<u32>{
#include "spv/bloomUp.comp.spv"
		}, "bloomUp.comp");
		_ptc.ctx.create_named_pipeline("bloom_up", bloomUpPci);
		
		pipelinesCreated = true;
		
	}
	
}

auto Bloom::apply(vuk::Name _target) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Bloom downsample",
		.resources = {
			vuk::Resource(_target, vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(Bloom_n, vuk::Resource::Type::eImage, vuk::eComputeRW),
		},
		.execute = [this, _target](vuk::CommandBuffer& cmd) {
			
			for (auto i = 0u; i < BloomPasses; i += 1) {
				
				if (i == 0) {
					
					cmd.bind_sampled_image(0, 0, _target, LinearClamp);
					cmd.bind_compute_pipeline("bloom_down_karis");
					
				} else {
					
					cmd.image_barrier(Bloom_n, vuk::eComputeRW, vuk::eComputeSampled, i-1, 1);
					cmd.bind_sampled_image(0, 0, *bloomViews[i-1], LinearClamp);
					cmd.bind_compute_pipeline("bloom_down");
					
				}
				cmd.bind_storage_image(0, 1, *bloomViews[i]);
				
				cmd.dispatch_invocations(size.width >> (i + 1), size.height >> (i + 1), 1);
				
			}
			
			cmd.image_barrier(Bloom_n, vuk::eComputeSampled, vuk::eComputeRW, 0, BloomPasses - 1);
			
		},
	});
	
	rg.add_pass({
		.name = "Bloom upsample",
		.resources = {
			vuk::Resource(Bloom_n, vuk::Resource::Type::eImage, vuk::eComputeRW),
			vuk::Resource(_target, vuk::Resource::Type::eImage, vuk::eComputeWrite),
		},
		.execute = [this, _target](vuk::CommandBuffer& cmd) {
			
			for (auto _i = 0u; _i < BloomPasses; _i += 1) {
				auto i = BloomPasses - _i - 1;
				
				cmd.image_barrier(Bloom_n, vuk::eComputeRW, vuk::eComputeSampled, i, 1);
				cmd.bind_sampled_image(0, 0, *bloomViews[i], LinearClamp);
				if (i == 0) {
					
					cmd.bind_storage_image(0, 1, _target);
					cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, BloomStrength);
					
				} else {
					
					cmd.bind_storage_image(0, 1, *bloomViews[i-1]);
					cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, 1.0f);
					
				}
				
				cmd.bind_compute_pipeline("bloom_up");
				cmd.dispatch_invocations(size.width >> i, size.height >> i, 1);
				
			}
			
		},
	});
	
	rg.attach_image(Bloom_n,
		vuk::ImageAttachment::from_texture(bloom),
		vuk::eNone,
		vuk::eNone);
	
	return rg;
	
}

}
