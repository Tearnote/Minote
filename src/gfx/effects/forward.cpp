#include "gfx/effects/forward.hpp"

#include "vuk/CommandBuffer.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void Forward::compile(vuk::PerThreadContext& _ptc) {
	
	auto zPrepassPci = vuk::PipelineBaseCreateInfo();
	zPrepassPci.add_spirv(std::vector<u32>{
#include "spv/zprepass.vert.spv"
	}, "zprepass.vert");
	zPrepassPci.add_spirv(std::vector<u32>{
#include "spv/zprepass.frag.spv"
	}, "zprepass.frag");
	zPrepassPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	zPrepassPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eGreater;
	_ptc.ctx.create_named_pipeline("z_prepass", zPrepassPci);
	
	auto forwardPci = vuk::PipelineBaseCreateInfo();
	forwardPci.add_spirv(std::vector<u32>{
#include "spv/forward.vert.spv"
	}, "forward.vert");
	forwardPci.add_spirv(std::vector<u32>{
#include "spv/forward.frag.spv"
	}, "forward.frag");
	forwardPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	forwardPci.depth_stencil_state.depthWriteEnable = false;
	forwardPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eEqual;
	_ptc.ctx.create_named_pipeline("forward", forwardPci);
	
}

void Forward::zPrepass(vuk::RenderGraph& _rg, Texture2D _depth, Buffer<World> _world,
	Indirect const& _indirect, MeshBuffer const& _meshes) {
	
	_rg.add_pass({
		.name = nameAppend(_depth.name, "z-prepass"),
		.resources = {
			_indirect.commandsBuf.resource(vuk::eIndirectRead),
			_indirect.transformsCulledBuf.resource(vuk::eVertexRead),
			_depth.resource(vuk::eDepthStencilRW) },
		.execute = [_depth, _world, &_indirect, &_meshes](vuk::CommandBuffer& cmd) {
			
			cmdSetViewportScissor(cmd, _depth.size());
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_vertex_buffer(0, _meshes.verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_index_buffer(_meshes.indicesBuf, vuk::IndexType::eUint16)
			   .bind_storage_buffer(0, 1, _indirect.transformsCulledBuf)
			   .bind_graphics_pipeline("z_prepass");
			
			cmd.draw_indexed_indirect(_indirect.commandsCount, _indirect.commandsBuf);
			
		}});
	
}

void Forward::draw(vuk::RenderGraph& _rg, Texture2D _color, Texture2D _depth,
	Buffer<World> _world, Indirect const& _indirect, MeshBuffer const& _meshes,
	Sky const& _sky, Cubemap _ibl) {
	
	_rg.add_pass({
		.name = nameAppend(_color.name, "forward"),
		.resources = {
			_indirect.commandsBuf.resource(vuk::eIndirectRead),
			_indirect.transformsCulledBuf.resource(vuk::eVertexRead),
			_indirect.materialsCulledBuf.resource(vuk::eVertexRead),
			_ibl.resource(vuk::eFragmentSampled),
			_sky.aerialPerspective.resource(vuk::eFragmentSampled),
			_sky.sunLuminance.resource(vuk::eFragmentRead),
			_color.resource(vuk::eColorWrite),
			_depth.resource(vuk::eDepthStencilRW) },
		.execute = [_color, _world, &_indirect, &_meshes, &_sky, _ibl](vuk::CommandBuffer& cmd) {
			
			cmdSetViewportScissor(cmd, _color.size());
			   
			cmd.bind_vertex_buffer(0, _meshes.verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(1, _meshes.normalsBuf,  1, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(2, _meshes.colorsBuf,   2, vuk::Packed{vuk::Format::eR16G16B16A16Unorm})
			   .bind_index_buffer(_meshes.indicesBuf, vuk::IndexType::eUint16)
			   
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _indirect.transformsCulledBuf)
			   .bind_storage_buffer(0, 2, _indirect.materialsCulledBuf)
			   .bind_storage_buffer(0, 3, _sky.sunLuminance)
			   .bind_sampled_image(0, 4, _ibl, TrilinearClamp)
			   .bind_sampled_image(0, 5, _sky.aerialPerspective, TrilinearClamp)
			   .bind_graphics_pipeline("forward");
			
			cmd.draw_indexed_indirect(_indirect.commandsCount, _indirect.commandsBuf);
			
		}});
	
}

}
