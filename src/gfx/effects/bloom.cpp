#include "gfx/effects/bloom.hpp"

#include <cassert>
#include "vuk/CommandBuffer.hpp"
#include "base/containers/string.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

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

auto Bloom::apply(Pool& _pool, Texture2D _target) -> vuk::RenderGraph {
	
	assert(_target.size().x() >= (1u << BloomPasses));
	assert(_target.size().y() >= (1u << BloomPasses));
	
	// Create temporary resources
	
	auto rg = vuk::RenderGraph();
	
	auto bloomTemp = Texture2D::make(_pool, nameAppend(_target.name, "bloomTemp"),
		_target.size() / 2u, BloomFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled,
		BloomPasses);
	bloomTemp.attach(rg, vuk::eNone, vuk::eNone);
	
	// Downsample pass: repeatedly draw the source image into increasingly smaller mips
	rg.add_pass({
		.name = nameAppend(_target.name, "bloom down"),
		.resources = {
			_target.resource(vuk::eComputeSampled),
			bloomTemp.resource(vuk::eComputeRW) },
		.execute = [_target, temp=bloomTemp](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(0u, BloomPasses)) {
				
				if (i == 0) { // First pass, read from target with lowpass filter
					
					cmd.bind_sampled_image(0, 0, _target, LinearClamp);
					cmd.bind_compute_pipeline("bloom_down_karis");
					
				} else { // Read from intermediate mip
					
					cmd.image_barrier(temp.name, vuk::eComputeRW, vuk::eComputeSampled, i-1, 1);
					cmd.bind_sampled_image(0, 0, *temp.mipView(i - 1), LinearClamp);
					cmd.bind_compute_pipeline("bloom_down");
					
				}
				cmd.bind_storage_image(0, 1, *temp.mipView(i));
				
				cmd.dispatch_invocations(_target.size().x() >> (i + 1), _target.size().y() >> (i + 1), 1);
				
			}
			
			// Mipmap usage requires manual barrier management
			cmd.image_barrier(temp.name, vuk::eComputeSampled, vuk::eComputeRW, 0, BloomPasses - 1);
			
		}});
	
	// Upsample pass: same as downsample, but in reverse order
	rg.add_pass({
		.name = nameAppend(_target.name, "bloom up"),
		.resources = {
			bloomTemp.resource(vuk::eComputeRW),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_target, temp=bloomTemp](vuk::CommandBuffer& cmd) {
			
			for (auto i: iota(0u, BloomPasses) | reverse) {
				
				cmd.image_barrier(temp.name, vuk::eComputeRW, vuk::eComputeSampled, i, 1);
				cmd.bind_sampled_image(0, 0, *temp.mipView(i), LinearClamp);
				if (i == 0) { // Final pass, draw to target
					
					cmd.bind_storage_image(0, 1, _target);
					cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, BloomStrength);
					
				} else { // Draw to intermediate mip
					
					cmd.bind_storage_image(0, 1, *temp.mipView(i - 1));
					cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, 1.0f);
					
				}
				
				cmd.bind_compute_pipeline("bloom_up");
				cmd.dispatch_invocations(_target.size().x() >> i, _target.size().y() >> i, 1);
				
			}
			
		}});
	
	return rg;
	
}

}
