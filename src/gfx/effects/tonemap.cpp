#include "gfx/effects/tonemap.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;

void Tonemap::compile(vuk::PerThreadContext& _ptc) {
	
	auto tonemapPci = vuk::ComputePipelineCreateInfo();
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.comp.spv"
	}, "blit.comp");
	_ptc.ctx.create_named_pipeline("tonemap", tonemapPci);
	
}

void Tonemap::apply(vuk::RenderGraph& _rg, Texture2D _source, Texture2D _target) {
	
	_rg.add_pass({
		.name = nameAppend(_source.name, "tonemapping"),
		.resources = {
			_source.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_source, _target](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _source, NearestClamp)
			   .bind_storage_image(0, 1, _target)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _target.size())
			   .bind_compute_pipeline("tonemap");
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y());
			
		}});
	
}

}
