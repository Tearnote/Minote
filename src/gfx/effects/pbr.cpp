#include "gfx/effects/pbr.hpp"

#include <cassert>
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base::literals;

void PBR::compile(vuk::PerThreadContext& _ptc) {
	
	auto pbrPci = vuk::ComputePipelineBaseCreateInfo();
	pbrPci.add_spirv(std::vector<u32>{
#include "spv/pbr.comp.spv"
	}, "pbr.comp");
	_ptc.ctx.create_named_pipeline("pbr", pbrPci);
	
	auto pbrQuadPci = vuk::ComputePipelineBaseCreateInfo();
	pbrQuadPci.add_spirv(std::vector<u32>{
#include "spv/pbrQuad.comp.spv"
	}, "pbrQuad.comp");
	_ptc.ctx.create_named_pipeline("pbr_quad", pbrQuadPci);
	
}

void PBR::apply(vuk::RenderGraph& _rg, Texture2D _color, Texture2D _visbuf,
	Worklist _worklist, Buffer<World> _world, ModelBuffer const& _models,
	DrawableInstanceList _instances, Cubemap _ibl, Buffer<vec3> _sunLuminance, Texture3D _aerialPerspective) {
	
	assert(_color.size() == _visbuf.size());
	
	_rg.add_pass({
		.name = nameAppend(_color.name, "pbr"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_worklist.counts.resource(vuk::eIndirectRead),
			_worklist.lists.resource(vuk::eComputeRead),
			_instances.instances.resource(vuk::eComputeRead),
			_instances.colors.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_sunLuminance.resource(vuk::eComputeRead),
			_aerialPerspective.resource(vuk::eComputeSampled),
			_ibl.resource(vuk::eComputeSampled),
			_color.resource(vuk::eComputeWrite) },
		.execute = [_color, _visbuf, _worklist, _world, &_models,
			_instances, _ibl, _sunLuminance, _aerialPerspective,
			tileCount=_worklist.counts.offsetView(+MaterialType::PBR)](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _models.meshes)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.colors)
			   .bind_storage_buffer(0, 4, _instances.transforms)
			   .bind_storage_buffer(0, 5, _models.indices)
			   .bind_storage_buffer(0, 6, _models.vertices)
			   .bind_storage_buffer(0, 7, _models.normals)
			   .bind_storage_buffer(0, 8, _models.materials)
			   .bind_uniform_buffer(0, 9, _sunLuminance)
			   .bind_sampled_image(0, 10, _ibl, TrilinearClamp)
			   .bind_sampled_image(0, 11, _aerialPerspective, TrilinearClamp)
			   .bind_sampled_image(0, 12, _visbuf, NearestClamp)
			   .bind_storage_image(0, 13, _color)
			   .bind_storage_buffer(0, 14, _worklist.lists)
			   .bind_compute_pipeline("pbr");
			
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16({_aerialPerspective.size().x(), _aerialPerspective.size().y()}));
			cmd.specialization_constants(1, vuk::ShaderStageFlagBits::eCompute, _aerialPerspective.size().z());
			cmd.specialization_constants(2, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_color.size()));
			cmd.specialization_constants(3, vuk::ShaderStageFlagBits::eCompute, _worklist.tileDimensions.x() * _worklist.tileDimensions.y() * +MaterialType::PBR);
			
			cmd.dispatch_indirect(tileCount);
			
		}});
	
}

void PBR::applyQuad(vuk::RenderGraph& _rg, QuadBuffer& _quadbuf,
	Worklist _worklist, Buffer<World> _world, ModelBuffer const& _models,
	DrawableInstanceList _instances, Cubemap _ibl,
	Buffer<vec3> _sunLuminance, Texture3D _aerialPerspective) {
	
	_rg.add_pass({
		.name = nameAppend(_quadbuf.name, "pbr_quad"),
		.resources = {
			_worklist.counts.resource(vuk::eIndirectRead),
			_worklist.lists.resource(vuk::eComputeRead),
			_instances.instances.resource(vuk::eComputeRead),
			_instances.colors.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_sunLuminance.resource(vuk::eComputeRead),
			_aerialPerspective.resource(vuk::eComputeSampled),
			_ibl.resource(vuk::eComputeSampled),
			_quadbuf.clusterDef.resource(vuk::eComputeSampled),
			_quadbuf.clusterOut.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, _worklist, _world, &_models, _instances, _ibl,
			_sunLuminance, _aerialPerspective,
			tileCount=_worklist.counts.offsetView(+MaterialType::PBR)](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _models.meshes)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.colors)
			   .bind_storage_buffer(0, 4, _instances.transforms)
			   .bind_storage_buffer(0, 5, _models.indices)
			   .bind_storage_buffer(0, 6, _models.vertices)
			   .bind_storage_buffer(0, 7, _models.normals)
			   .bind_storage_buffer(0, 8, _models.materials)
			   .bind_uniform_buffer(0, 9, _sunLuminance)
			   .bind_sampled_image(0, 10, _ibl, TrilinearClamp)
			   .bind_sampled_image(0, 11, _aerialPerspective, TrilinearClamp)
			   .bind_sampled_image(0, 12, _quadbuf.clusterDef, NearestClamp)
			   .bind_storage_image(0, 13, _quadbuf.clusterOut)
			   .bind_storage_buffer(0, 14, _worklist.lists)
			   .bind_compute_pipeline("pbr_quad");
			
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16({_aerialPerspective.size().x(), _aerialPerspective.size().y()}));
			cmd.specialization_constants(1, vuk::ShaderStageFlagBits::eCompute, _aerialPerspective.size().z());
			cmd.specialization_constants(2, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_quadbuf.clusterOut.size()));
			cmd.specialization_constants(3, vuk::ShaderStageFlagBits::eCompute, _worklist.tileDimensions.x() * _worklist.tileDimensions.y() * +MaterialType::PBR);
			
			cmd.dispatch_indirect(tileCount);
			
		}});
	
}

}
