#include "gfx/modules/forward.hpp"

#include "vuk/CommandBuffer.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx::modules {

Forward::Forward(vuk::PerThreadContext& _ptc, vuk::Extent2D _targetSize) {
	
	size = _targetSize;
	
	if (!pipelinesCreated) {
		
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
		
		auto objectPci = vuk::PipelineBaseCreateInfo();
		objectPci.add_spirv(std::vector<u32>{
#include "spv/object.vert.spv"
		}, "object.vert");
		objectPci.add_spirv(std::vector<u32>{
#include "spv/object.frag.spv"
		}, "object.frag");
		objectPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
		objectPci.depth_stencil_state.depthWriteEnable = false;
		objectPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eEqual;
		_ptc.ctx.create_named_pipeline("object", objectPci);
		
		pipelinesCreated = true;
		
	}
	
}

auto Forward::zPrepass(vuk::Buffer _world, Indirect& _indirect, Meshes& _meshes) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Z-prepass",
		.resources = {
			vuk::Resource(_indirect.Commands_n,        vuk::Resource::Type::eBuffer, vuk::eIndirectRead),
			vuk::Resource(_indirect.InstancesCulled_n, vuk::Resource::Type::eBuffer, vuk::eVertexRead),
			vuk::Resource(Depth_n,                     vuk::Resource::Type::eImage,  vuk::eDepthStencilRW),
		},
		.execute = [this, _world, &_indirect, &_meshes](vuk::CommandBuffer& cmd) {
			cmd.set_viewport(0, vuk::Rect2D{ .extent = size })
			   .set_scissor(0, vuk::Rect2D{ .extent = size })
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_vertex_buffer(0, *_meshes.verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_index_buffer(*_meshes.indicesBuf, vuk::IndexType::eUint16)
			   .bind_storage_buffer(0, 1, _indirect.instancesCulledBuf)
			   .bind_graphics_pipeline("z_prepass");
			cmd.draw_indexed_indirect(_indirect.commandsCount, _indirect.commandsBuf, sizeof(Indirect::Command));
		},
	});
	
	rg.attach_managed(Depth_n,
		DepthFormat,
		vuk::Dimension2D::absolute(size),
		SampleCount,
		vuk::ClearDepthStencil{0.0f, 0});
	
	return rg;
	
}

auto Forward::draw(vuk::Buffer _world, Indirect& _indirect,
	Meshes& _meshes, Sky& _sky, IBLMap& _ibl) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Object drawing",
		.resources = {
			vuk::Resource(_indirect.Commands_n,        vuk::Resource::Type::eBuffer, vuk::eIndirectRead),
			vuk::Resource(_indirect.InstancesCulled_n, vuk::Resource::Type::eBuffer, vuk::eVertexRead),
			vuk::Resource(_ibl.Filtered_n,             vuk::Resource::Type::eImage,  vuk::eFragmentSampled),
			vuk::Resource(_sky.AerialPerspective_n,    vuk::Resource::Type::eImage,  vuk::eFragmentSampled),
			vuk::Resource(_sky.SunLuminance_n,         vuk::Resource::Type::eBuffer, vuk::eFragmentRead),
			vuk::Resource(Color_n,                     vuk::Resource::Type::eImage,  vuk::eColorWrite),
			vuk::Resource(Depth_n,                     vuk::Resource::Type::eImage,  vuk::eDepthStencilRW),
		},
		.execute = [this, _world, &_indirect, &_meshes, &_sky, &_ibl](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D{ .extent = size })
			   .set_scissor(0, vuk::Rect2D{ .extent = size })
			   
			   .bind_vertex_buffer(0, *_meshes.verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(1, *_meshes.normalsBuf,  1, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(2, *_meshes.colorsBuf,   2, vuk::Packed{vuk::Format::eR16G16B16A16Unorm})
			   .bind_index_buffer(*_meshes.indicesBuf, vuk::IndexType::eUint16)
			   
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_storage_buffer(0, 1, _indirect.instancesCulledBuf)
			   .bind_storage_buffer(0, 2, *_sky.sunLuminance)
			   .bind_sampled_image(0, 3, _ibl.Filtered_n, TrilinearClamp)
			   .bind_sampled_image(0, 4, _sky.AerialPerspective_n, TrilinearClamp)
			   .bind_graphics_pipeline("object");
			
			cmd.draw_indexed_indirect(_indirect.commandsCount, _indirect.commandsBuf, sizeof(Indirect::Command));
		},
	});
	
	rg.attach_managed(Color_n,
		ColorFormat,
		vuk::Dimension2D::absolute(size),
		SampleCount,
		vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	
	return rg;
	
}

auto Forward::resolve() -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.resolve_resource_into(Resolved_n, Color_n);
	
	rg.attach_managed(Resolved_n,
		ColorFormat,
		vuk::Dimension2D::absolute(size),
		vuk::Samples::e1,
		vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
		
	return rg;
	
}

}
