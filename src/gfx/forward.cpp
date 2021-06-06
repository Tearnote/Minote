#include "gfx/forward.hpp"

#include "vuk/CommandBuffer.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

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
			"commands"_buffer(vuk::eIndirectRead),
			"instances_culled"_buffer(vuk::eVertexRead),
			"object_depth"_image(vuk::eDepthStencilRW),
		},
		.execute = [this, _world, &_indirect, &_meshes](vuk::CommandBuffer& cmd) {
			auto commandsBuf = cmd.get_resource_buffer("commands");
			auto instancesBuf = cmd.get_resource_buffer("instances_culled");
			cmd.set_viewport(0, vuk::Rect2D{ .extent = size })
			   .set_scissor(0, vuk::Rect2D{ .extent = size })
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_vertex_buffer(0, *_meshes.verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_index_buffer(*_meshes.indicesBuf, vuk::IndexType::eUint16)
			   .bind_storage_buffer(0, 1, instancesBuf)
			   .bind_graphics_pipeline("z_prepass");
			cmd.draw_indexed_indirect(_indirect.commandsCount, commandsBuf, sizeof(Indirect::Command));
		},
	});
	
	rg.attach_managed("object_depth",
		vuk::Format::eD32Sfloat,
		vuk::Dimension2D::absolute(size),
		vuk::Samples::e4,
		vuk::ClearDepthStencil{0.0f, 0});
	
	return rg;
	
}

auto Forward::draw(vuk::Buffer _world, Indirect& _indirect, Meshes& _meshes) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Object drawing",
		.resources = {
			"commands"_buffer(vuk::eIndirectRead),
			"instances_culled"_buffer(vuk::eVertexRead),
			"ibl_map_filtered"_image(vuk::eFragmentSampled),
			"sky_aerial_perspective"_image(vuk::eFragmentSampled),
			"sky_sun_luminance"_buffer(vuk::eFragmentRead),
			"object_color"_image(vuk::eColorWrite),
			"object_depth"_image(vuk::eDepthStencilRW),
		},
		.execute = [this, _world, &_indirect, &_meshes](vuk::CommandBuffer& cmd) {
			auto commandsBuf = cmd.get_resource_buffer("commands");
			auto instancesBuf = cmd.get_resource_buffer("instances_culled");
			auto sunLuminanceBuf = cmd.get_resource_buffer("sky_sun_luminance");
			cmd.set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_vertex_buffer(0, *_meshes.verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(1, *_meshes.normalsBuf, 1, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(2, *_meshes.colorsBuf, 2, vuk::Packed{vuk::Format::eR16G16B16A16Unorm})
			   .bind_index_buffer(*_meshes.indicesBuf, vuk::IndexType::eUint16)
			   .bind_storage_buffer(0, 1, instancesBuf)
			   .bind_storage_buffer(0, 2, sunLuminanceBuf)
			   .bind_sampled_image(0, 3, "ibl_map_filtered", TrilinearClamp)
			   .bind_sampled_image(0, 4, "sky_aerial_perspective", TrilinearClamp)
			   .bind_graphics_pipeline("object");
			cmd.draw_indexed_indirect(_indirect.commandsCount, commandsBuf, sizeof(Indirect::Command));
		},
	});
	
	rg.attach_managed("object_color",
		vuk::Format::eR16G16B16A16Sfloat,
		vuk::Dimension2D::absolute(size),
		vuk::Samples::e4,
		vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	rg.attach_managed("object_resolved",
		vuk::Format::eR16G16B16A16Sfloat,
		vuk::Dimension2D::absolute(size),
		vuk::Samples::e1,
		vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	
	return rg;
	
}

}
