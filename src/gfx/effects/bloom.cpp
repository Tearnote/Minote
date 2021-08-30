#include "gfx/effects/bloom.hpp"

#include <cassert>
#include "vuk/CommandBuffer.hpp"
#include "base/containers/string.hpp"
#include "base/types.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;

void Bloom::compile(vuk::PerThreadContext& _ptc) {
	
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
	
}

auto Bloom::apply(vuk::PerThreadContext& _ptc, string_view _name, vuk::Name _dst, uvec2 _size) -> vuk::RenderGraph {
	
	assert(_size.x() >= (1 << BloomPasses));
	assert(_size.y() >= (1 << BloomPasses));
	
	// Create temporary resources
	
	auto* Bloom_n = "Bloom::bloomTex";
	auto bloomTex = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = BloomFormat,
		.extent = {_size.x() >> 1, _size.y() >> 1, 1},
		.mipLevels = BloomPasses,
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled });
	auto bloomViews = array<vuk::ImageView, BloomPasses>();
	
	auto rg = vuk::RenderGraph();
	
	for (auto i: iota(0u, BloomPasses)) {
		
		bloomViews[i] = _ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *bloomTex.image,
			.format = bloomTex.format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1 }}).release();
		
	}
	
	// Downsample pass: repeatedly draw the source image into increasingly smaller mips
	rg.add_pass({
		.name = vuk::Name(string(_name) + " downsample"),
		.resources = {
			vuk::Resource(_dst,    vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(Bloom_n, vuk::Resource::Type::eImage, vuk::eComputeRW) },
		.execute = [_dst, Bloom_n, _size, bloomViews](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(0u, BloomPasses)) {
				
				if (i == 0) { // First pass, read from target with lowpass filter
					
					cmd.bind_sampled_image(0, 0, _dst, LinearClamp);
					cmd.bind_compute_pipeline("bloom_down_karis");
					
				} else { // Read from intermediate mip
					
					cmd.image_barrier(Bloom_n, vuk::eComputeRW, vuk::eComputeSampled, i-1, 1);
					cmd.bind_sampled_image(0, 0, bloomViews[i-1], LinearClamp);
					cmd.bind_compute_pipeline("bloom_down");
					
				}
				cmd.bind_storage_image(0, 1, bloomViews[i]);
				
				cmd.dispatch_invocations(_size.x() >> (i + 1), _size.y() >> (i + 1), 1);
				
			}
			
			// Mipmap usage requires manual barrier management
			cmd.image_barrier(Bloom_n, vuk::eComputeSampled, vuk::eComputeRW, 0, BloomPasses - 1);
			
		}});
	
	// Upsample pass: same as downsample, but in reverse order
	rg.add_pass({
		.name = vuk::Name(string(_name) + " upsample"),
		.resources = {
			vuk::Resource(Bloom_n, vuk::Resource::Type::eImage, vuk::eComputeRW),
			vuk::Resource(_dst,    vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [_dst, Bloom_n, _size, bloomViews](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(0u, BloomPasses) | reverse) {
				
				cmd.image_barrier(Bloom_n, vuk::eComputeRW, vuk::eComputeSampled, i, 1);
				cmd.bind_sampled_image(0, 0, bloomViews[i], LinearClamp);
				if (i == 0) { // Final pass, draw to target
					
					cmd.bind_storage_image(0, 1, _dst);
					cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, BloomStrength);
					
				} else { // Draw to intermediate mip
					
					cmd.bind_storage_image(0, 1, bloomViews[i-1]);
					cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, 1.0f);
					
				}
				
				cmd.bind_compute_pipeline("bloom_up");
				cmd.dispatch_invocations(_size.x() >> i, _size.y() >> i, 1);
				
			}
			
		}});
	
	rg.attach_image(Bloom_n,
		vuk::ImageAttachment::from_texture(bloomTex),
		vuk::eNone,
		vuk::eNone);
	
	// Recycle temporary resources
	
	_ptc.ctx.enqueue_destroy(bloomTex.image.release());
	_ptc.ctx.enqueue_destroy(bloomTex.view.release());
	for (auto iv: bloomViews)
		_ptc.ctx.enqueue_destroy(iv);
	
	return rg;
	
}

}
