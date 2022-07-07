#include "gfx/effects/tonemap.hpp"

#include "vuk/CommandBuffer.hpp"
#include "util/types.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote {

void Tonemap::compile(vuk::PerThreadContext& _ptc) {
	
	auto tonemapPci = vuk::ComputePipelineBaseCreateInfo();
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.comp.spv"
	}, "tonemap.comp");
	_ptc.ctx.create_named_pipeline("tonemap", tonemapPci);
	
}

void Tonemap::apply(Frame& _frame, Texture2D _source, Texture2D _target) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_source.name, "tonemapping"),
		.resources = {
			_source.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_source, _target](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _source, NearestClamp)
			   .bind_storage_image(0, 1, _target)
			   .bind_compute_pipeline("tonemap");
			
			cmd.specialize_constants(0, u32Fromu16(_target.size()));
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y());
			
		}});
	
}

}
