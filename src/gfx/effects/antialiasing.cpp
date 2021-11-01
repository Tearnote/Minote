#include "gfx/effects/antialiasing.hpp"

#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void Antialiasing::compile(vuk::PerThreadContext& _ptc) {
	
	auto quadScatterPci = vuk::ComputePipelineBaseCreateInfo();
	quadScatterPci.add_spirv(std::vector<u32>{
#include "spv/quadScatter.comp.spv"
	}, "quadScatter.comp");
	_ptc.ctx.create_named_pipeline("quad_scatter", quadScatterPci);
	
	auto quadResolvePci = vuk::ComputePipelineBaseCreateInfo();
	quadResolvePci.add_spirv(std::vector<u32>{
#include "spv/quadResolve.comp.spv"
	}, "quadResolve.comp");
	_ptc.ctx.create_named_pipeline("quad_resolve", quadResolvePci);
	
}

void Antialiasing::quadScatter(vuk::RenderGraph& _rg, Texture2DMS _visbuf, Texture2D _quadbuf, Buffer<World> _world) {
	
	_rg.add_pass({
		.name = nameAppend(_visbuf.name, "Quad scatter"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_quadbuf.resource(vuk::eComputeWrite) },
		.execute = [_visbuf, _quadbuf, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_sampled_image(0, 1, _visbuf, NearestClamp)
			   .bind_storage_image(0, 2, _quadbuf)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _visbuf.size())
			   .bind_compute_pipeline("quad_scatter");
			
			auto invocationCount = _visbuf.size() / 2u + _visbuf.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
}

void Antialiasing::quadResolve(vuk::RenderGraph& _rg, Texture2D _target,
	Texture2D _quadbuf, Texture2D _outputs, Texture2D _history, Buffer<World> _world) {
	
	_rg.add_pass({
		.name = nameAppend(_quadbuf.name, "Quad resolve"),
		.resources = {
			_quadbuf.resource(vuk::eComputeSampled),
			_outputs.resource(vuk::eComputeSampled),
			_history.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, _outputs, _history, _target, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_sampled_image(0, 1, _quadbuf, NearestClamp)
			   .bind_sampled_image(0, 2, _outputs, NearestClamp)
			   .bind_sampled_image(0, 3, _history, LinearClamp)
			   .bind_storage_image(0, 4, _target)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _quadbuf.size())
			   .bind_compute_pipeline("quad_resolve");
			
			auto invocationCount = _quadbuf.size() / 2u + _quadbuf.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
}

}
