#include "gfx/effects/antialiasing.hpp"

#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void Antialiasing::compile(vuk::PerThreadContext& _ptc) {
	
	auto quadResolvePci = vuk::ComputePipelineCreateInfo();
	quadResolvePci.add_spirv(std::vector<u32>{
#include "spv/quadResolve.comp.spv"
	}, "quadResolve.comp");
	_ptc.ctx.create_named_pipeline("quad_resolve", quadResolvePci);
	
}

void Antialiasing::resolveQuad(vuk::RenderGraph& _rg, Texture2DMS _visbuf, Texture2D _resolved, Buffer<World> _world) {
	
	_rg.add_pass({
		.name = nameAppend(_visbuf.name, "Quad resolve"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_resolved.resource(vuk::eComputeWrite) },
		.execute = [_visbuf, _resolved, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_sampled_image(0, 1, _visbuf, NearestClamp)
			   .bind_storage_image(0, 2, _resolved)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _visbuf.size())
			   .bind_compute_pipeline("quad_resolve");
			
			cmd.dispatch_invocations(_visbuf.size().x(), _visbuf.size().y());
			
	}});
	
}

}
