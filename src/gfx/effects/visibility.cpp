#include "gfx/effects/visibility.hpp"

#include "vuk/CommandBuffer.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void Visibility::compile(vuk::PerThreadContext& _ptc) {
	
	auto visibilityPci = vuk::PipelineBaseCreateInfo();
	visibilityPci.add_spirv(std::vector<u32>{
#include "spv/visibility.vert.spv"
	}, "visibility.vert");
	visibilityPci.add_spirv(std::vector<u32>{
#include "spv/visibility.frag.spv"
	}, "visibility.frag");
	visibilityPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	visibilityPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eGreater;
	_ptc.ctx.create_named_pipeline("visibility", visibilityPci);
	
}

void Visibility::apply(vuk::RenderGraph& _rg, Texture2D _visbuf, Texture2D _depth,
	Buffer<World> _world, DrawableInstanceList const& _instances, MeshBuffer const& _meshes) {
	
	_rg.add_pass({
		.name = nameAppend(_visbuf.name, "visibility"),
		.resources = {
			_instances.commands.resource(vuk::eIndirectRead),
			_instances.meshIndices.resource(vuk::eIndexRead),
			_instances.transforms.resource(vuk::eVertexRead),
			_visbuf.resource(vuk::eColorWrite),
			_depth.resource(vuk::eDepthStencilRW) },
		.execute = [_visbuf, _world, &_instances, &_meshes](vuk::CommandBuffer& cmd) {
			
			cmdSetViewportScissor(cmd, _visbuf.size());
			cmd.bind_index_buffer(_meshes.indicesBuf, vuk::IndexType::eUint16)
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _meshes.verticesBuf)
			   .bind_storage_buffer(0, 2, _instances.transforms)
			   .bind_graphics_pipeline("visibility");
			
			cmd.draw_indexed_indirect(_instances.commands.length(), _instances.commands);
			
		}});
	
}

}
