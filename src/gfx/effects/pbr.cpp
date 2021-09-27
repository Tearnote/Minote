#include "gfx/effects/pbr.hpp"

#include <cassert>
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void PBR::compile(vuk::PerThreadContext& _ptc) {
	
	auto pbrPci = vuk::ComputePipelineCreateInfo();
	pbrPci.add_spirv(std::vector<u32>{
#include "spv/pbr.comp.spv"
	}, "pbr.comp");
	_ptc.ctx.create_named_pipeline("pbr", pbrPci);
	
}

void PBR::apply(vuk::RenderGraph& _rg, Texture2D _color, Texture2D _visbuf, Texture2D _depth,
	Buffer<World> _world, MeshBuffer const& _meshes, DrawableInstanceList const& _instances,
	Sky const& _sky, Cubemap _ibl) {
	
	assert(_color.size() == _visbuf.size());
	
	_rg.add_pass({
		.name = nameAppend(_color.name, "PBR"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_depth.resource(vuk::eComputeSampled),
			_instances.meshIndices.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_instances.materials.resource(vuk::eComputeRead),
			_sky.sunLuminance.resource(vuk::eComputeRead),
			_sky.aerialPerspective.resource(vuk::eComputeSampled),
			_ibl.resource(vuk::eComputeSampled),
			_color.resource(vuk::eComputeWrite) },
		.execute = [_color, _visbuf, _depth, _world, &_meshes, &_instances, &_sky, _ibl](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _meshes.descriptorBuf)
			   .bind_storage_buffer(0, 2, _instances.meshIndices)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_storage_buffer(0, 4, _instances.materials)
			   .bind_storage_buffer(0, 5, _meshes.indicesBuf)
			   .bind_storage_buffer(0, 6, _meshes.verticesBuf)
			   .bind_storage_buffer(0, 7, _meshes.normalsBuf)
			   .bind_storage_buffer(0, 8, _meshes.colorsBuf)
			   .bind_storage_buffer(0, 9, _sky.sunLuminance)
			   .bind_sampled_image(0, 10, _ibl, TrilinearClamp)
			   .bind_sampled_image(0, 11, _sky.aerialPerspective, TrilinearClamp)
			   .bind_sampled_image(0, 12, _visbuf, NearestClamp)
			   .bind_sampled_image(0, 13, _depth, NearestClamp)
			   .bind_storage_image(0, 14, _color)
			   .bind_compute_pipeline("pbr");
			
			struct PushConstants {
				uvec3 aerialPerspectiveSize;
				float pad0;
				uvec2 targetSize;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				.aerialPerspectiveSize = _sky.aerialPerspective.size(),
				.targetSize = _color.size() });
			
			cmd.dispatch_invocations(_color.size().x(), _color.size().y());
			
		}});
	
}

}
