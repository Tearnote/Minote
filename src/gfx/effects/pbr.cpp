#include "gfx/effects/pbr.hpp"

#include <cassert>
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base::literals;

void PBR::compile(vuk::PerThreadContext& _ptc) {
	
	auto pbrPci = vuk::ComputePipelineCreateInfo();
	pbrPci.add_spirv(std::vector<u32>{
#include "spv/pbr.comp.spv"
	}, "pbr.comp");
	_ptc.ctx.create_named_pipeline("pbr", pbrPci);
	
}

void PBR::apply(vuk::RenderGraph& _rg, Texture2D _color, Texture2D _visbuf, Texture2D _depth,
	Worklist const& _worklist, Buffer<World> _world, MeshBuffer const& _meshes, MaterialBuffer const& _materials,
	DrawableInstanceList const& _instances, Cubemap _ibl, Buffer<vec3> _sunLuminance, Texture3D _aerialPerspective) {
	
	assert(_color.size() == _visbuf.size());
	
	_rg.add_pass({
		.name = nameAppend(_color.name, "PBR"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_depth.resource(vuk::eComputeSampled),
			_worklist.counts.resource(vuk::eIndirectRead),
			_worklist.lists.resource(vuk::eComputeRead),
			_instances.instances.resource(vuk::eComputeRead),
			_instances.colors.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_sunLuminance.resource(vuk::eComputeRead),
			_aerialPerspective.resource(vuk::eComputeSampled),
			_ibl.resource(vuk::eComputeSampled),
			_color.resource(vuk::eComputeWrite) },
		.execute = [_color, _visbuf, _depth, &_worklist, _world, &_meshes, &_materials, &_instances,
			_ibl, _sunLuminance, _aerialPerspective](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _meshes.descriptorBuf)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.colors)
			   .bind_storage_buffer(0, 4, _instances.transforms)
			   .bind_storage_buffer(0, 5, _meshes.indicesBuf)
			   .bind_storage_buffer(0, 6, _meshes.verticesBuf)
			   .bind_storage_buffer(0, 7, _meshes.normalsBuf)
			   .bind_storage_buffer(0, 8, _meshes.colorsBuf)
			   .bind_storage_buffer(0, 9, _materials.materials)
			   .bind_uniform_buffer(0, 10, _sunLuminance)
			   .bind_sampled_image(0, 11, _ibl, TrilinearClamp)
			   .bind_sampled_image(0, 12, _aerialPerspective, TrilinearClamp)
			   .bind_sampled_image(0, 13, _visbuf, NearestClamp)
			   .bind_sampled_image(0, 14, _depth, NearestClamp)
			   .bind_storage_image(0, 15, _color)
			   .bind_storage_buffer(1, 0, _worklist.lists)
			   .bind_compute_pipeline("pbr");
			
			struct PushConstants {
				uvec3 aerialPerspectiveSize;
				u32 tileOffset;
				uvec2 targetSize;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				.aerialPerspectiveSize = _aerialPerspective.size(),
				.tileOffset = _worklist.tileDimensions.x() * _worklist.tileDimensions.y() * +MaterialType::PBR,
				.targetSize = _color.size() });
			
			cmd.dispatch_indirect(_worklist.counts, sizeof(uvec4) * +MaterialType::PBR);
			
		}});
	
}

}
