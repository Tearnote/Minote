#include "gfx/effects/antialiasing.hpp"

#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void Antialiasing::compile(vuk::PerThreadContext& _ptc) {
	
	auto quadResolvePci = vuk::ComputePipelineBaseCreateInfo();
	quadResolvePci.add_spirv(std::vector<u32>{
#include "spv/quadResolve.comp.spv"
	}, "quadResolve.comp");
	_ptc.ctx.create_named_pipeline("quad_resolve", quadResolvePci);
	
}

void Antialiasing::quadResolve(vuk::RenderGraph& _rg, Texture2D _target, Texture2D _velocity,
	Texture2D _quadbuf, Texture2D _jitterMap, Texture2D _outputs,
	Texture2D _targetPrev, Texture2D _quadbufPrev, Texture2D _outputsPrev, Texture2D _jitterMapPrev,
	Buffer<World> _world) {
	
	_rg.add_pass({
		.name = nameAppend(_quadbuf.name, "Quad resolve"),
		.resources = {
			_quadbuf.resource(vuk::eComputeSampled),
			_jitterMap.resource(vuk::eComputeSampled),
			_outputs.resource(vuk::eComputeSampled),
			_targetPrev.resource(vuk::eComputeSampled),
			_quadbufPrev.resource(vuk::eComputeSampled),
			_outputsPrev.resource(vuk::eComputeSampled),
			_jitterMapPrev.resource(vuk::eComputeSampled),
			_velocity.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, _jitterMap, _outputs, _targetPrev, _quadbufPrev, _outputsPrev, _jitterMapPrev,
			_target, _velocity, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_sampled_image(0, 1, _quadbuf, NearestClamp)
			   .bind_sampled_image(0, 2, _jitterMap, NearestClamp)
			   .bind_sampled_image(0, 3, _outputs, NearestClamp)
			   .bind_sampled_image(0, 4, _targetPrev, LinearClamp)
			   .bind_sampled_image(0, 5, _quadbufPrev, NearestClamp)
			   .bind_sampled_image(0, 6, _outputsPrev, NearestClamp)
			   .bind_sampled_image(0, 7, _jitterMapPrev, NearestClamp)
			   .bind_sampled_image(0, 8, _velocity, LinearClamp)
			   .bind_storage_image(0, 9, _target)
			   .specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_quadbuf.size()))
			   .bind_compute_pipeline("quad_resolve");
			
			auto invocationCount = _quadbuf.size() / 2u + _quadbuf.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
}

}
