#include "gfx/effects/antialiasing.hpp"

#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void Antialiasing::compile(vuk::PerThreadContext& _ptc) {
	
	auto quadAssignPci = vuk::ComputePipelineBaseCreateInfo();
	quadAssignPci.add_spirv(std::vector<u32>{
#include "spv/quadAssign.comp.spv"
	}, "quadAssign.comp");
	_ptc.ctx.create_named_pipeline("quad_assign", quadAssignPci);
	
	auto quadResolvePci = vuk::ComputePipelineBaseCreateInfo();
	quadResolvePci.add_spirv(std::vector<u32>{
#include "spv/quadResolve.comp.spv"
	}, "quadResolve.comp");
	_ptc.ctx.create_named_pipeline("quad_resolve", quadResolvePci);
	
}

void Antialiasing::quadAssign(vuk::RenderGraph& _rg, Texture2DMS _visbuf,
	Texture2D _quadbuf, Texture2D _jitterMap, Buffer<World> _world) {
	
	_rg.add_pass({
		.name = nameAppend(_visbuf.name, "Quad assign"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_quadbuf.resource(vuk::eComputeWrite),
			_jitterMap.resource(vuk::eComputeWrite) },
		.execute = [_visbuf, _jitterMap, _quadbuf, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_sampled_image(0, 1, _visbuf, NearestClamp)
			   .bind_storage_image(0, 2, _quadbuf)
			   .bind_storage_image(0, 3, _jitterMap)
			   .bind_compute_pipeline("quad_assign");
			
			auto invocationCount = _visbuf.size() / 2u + _visbuf.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
}

void Antialiasing::quadResolve(vuk::RenderGraph& _rg, Texture2D _target, Texture2D _velocity,
	Texture2D _quadbuf, Texture2D _jitterMap, Texture2D _outputs,
	Texture2D _targetPrev, Buffer<World> _world) {
	
	_rg.add_pass({
		.name = nameAppend(_quadbuf.name, "Quad resolve"),
		.resources = {
			_quadbuf.resource(vuk::eComputeSampled),
			_jitterMap.resource(vuk::eComputeSampled),
			_outputs.resource(vuk::eComputeSampled),
			_targetPrev.resource(vuk::eComputeSampled),
			_velocity.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, _jitterMap, _outputs, _targetPrev,
			_target, _velocity, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_sampled_image(0, 1, _quadbuf, NearestClamp)
			   .bind_sampled_image(0, 2, _jitterMap, NearestClamp)
			   .bind_sampled_image(0, 3, _outputs, NearestClamp)
			   .bind_sampled_image(0, 4, _targetPrev, LinearClamp)
			   .bind_sampled_image(0, 5, _velocity, LinearClamp)
			   .bind_storage_image(0, 6, _target)
			   .specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_quadbuf.size()))
			   .bind_compute_pipeline("quad_resolve");
			
			auto invocationCount = _quadbuf.size() / 2u + _quadbuf.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
}

}
