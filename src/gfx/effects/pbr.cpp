#include "gfx/effects/pbr.hpp"

#include <cassert>
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote {

void PBR::compile(vuk::PerThreadContext& _ptc) {
	
	auto pbrPci = vuk::ComputePipelineBaseCreateInfo();
	pbrPci.add_spirv(std::vector<u32>{
#include "spv/pbr.comp.spv"
	}, "pbr.comp");
	_ptc.ctx.create_named_pipeline("pbr", pbrPci);
	
}

void PBR::apply(Frame& _frame, QuadBuffer& _quadbuf, Worklist _worklist,
	TriangleList _triangles, Cubemap _ibl,
	Buffer<vec3> _sunLuminance, Texture3D _aerialPerspective) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_quadbuf.name, "pbr"),
		.resources = {
			_worklist.counts.resource(vuk::eIndirectRead),
			_worklist.lists.resource(vuk::eComputeRead),
			_triangles.indices.resource(vuk::eComputeRead),
			_triangles.instances.resource(vuk::eComputeRead),
			_triangles.colors.resource(vuk::eComputeRead),
			_sunLuminance.resource(vuk::eComputeRead),
			_aerialPerspective.resource(vuk::eComputeSampled),
			_ibl.resource(vuk::eComputeSampled),
			_quadbuf.visbuf.resource(vuk::eComputeSampled),
			_quadbuf.offset.resource(vuk::eComputeSampled),
			_quadbuf.depth.resource(vuk::eComputeSampled),
			_quadbuf.normal.resource(vuk::eComputeSampled),
			_quadbuf.clusterOut.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, _worklist, &_frame, _triangles, _ibl,
			_sunLuminance, _aerialPerspective,
			tileCount=_worklist.counts.offsetView(+MaterialType::PBR)](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_storage_buffer(0, 1, _frame.models.meshlets)
			   .bind_storage_buffer(0, 2, _triangles.indices)
			   .bind_storage_buffer(0, 3, _triangles.instances)
			   .bind_storage_buffer(0, 4, _triangles.colors)
			   .bind_storage_buffer(0, 5, _frame.models.materials)
			   .bind_uniform_buffer(0, 6, _sunLuminance)
			   .bind_sampled_image(0, 7, _ibl, TrilinearClamp)
			   .bind_sampled_image(0, 8, _aerialPerspective, TrilinearClamp)
			   .bind_sampled_image(0, 9, _quadbuf.visbuf, NearestClamp)
			   .bind_sampled_image(0, 10, _quadbuf.offset, NearestClamp)
			   .bind_sampled_image(0, 11, _quadbuf.depth, NearestClamp)
			   .bind_sampled_image(0, 12, _quadbuf.normal, NearestClamp)
			   .bind_storage_image(0, 13, _quadbuf.clusterOut)
			   .bind_storage_buffer(0, 14, _worklist.lists)
			   .bind_compute_pipeline("pbr");
			
			cmd.specialize_constants(0, u32Fromu16({_aerialPerspective.size().x(), _aerialPerspective.size().y()}));
			cmd.specialize_constants(1, _aerialPerspective.size().z());
			cmd.specialize_constants(2, u32Fromu16(_quadbuf.clusterOut.size()));
			cmd.specialize_constants(3, _worklist.tileDimensions.x() * _worklist.tileDimensions.y() * +MaterialType::PBR);
			
			cmd.dispatch_indirect(tileCount);
			
		}});
	
}

}
