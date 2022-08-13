#include "gfx/effects/visibility.hpp"

#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"

namespace minote {

Visibility::Visibility(vuk::Allocator& _allocator, ModelBuffer& _models,
	ObjectBuffer& _objects, InstanceList& _instances, TriangleList& _triangles,
	uint2 _extent, float4x4 _viewProjection) {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("visibility");
	rg->attach_in("indices", _triangles.indices);
	rg->attach_in("vertIndices", _models.vertIndices);
	rg->attach_in("vertices", _models.vertices);
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("transforms", _objects.transforms);
	rg->attach_in("instances", _instances.instances);
	rg->attach_in("command", _triangles.command);
	rg->attach_and_clear_image("visibility", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(_extent.x(), _extent.y()),
		.format = vuk::Format::eR32Uint,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	}, vuk::ClearColor(-1u, -1u, -1u, -1u));
	rg->attach_and_clear_image("depth", vuk::ImageAttachment{
		.format = vuk::Format::eD32Sfloat,
		.level_count = 1,
		.layer_count = 1,
	}, vuk::ClearDepth(0.0f));
	
	rg->add_pass(vuk::Pass{
		.name = "visibility/draw",
		.resources = {
			"indices"_buffer >> vuk::eIndexRead,
			"vertIndices"_buffer >> vuk::eVertexRead,
			"vertices"_buffer >> vuk::eVertexRead,
			"meshlets"_buffer >> vuk::eVertexRead,
			"transforms"_buffer >> vuk::eVertexRead,
			"instances"_buffer >> vuk::eVertexRead,
			"command"_buffer >> vuk::eIndirectRead,
			"visibility"_image >> vuk::eColorWrite >> "visibility/final",
			"depth"_image >> vuk::eDepthStencilRW >> "depth/final",
		},
		.execute = [_viewProjection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_graphics_pipeline("visibility/draw")
			   .set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .broadcast_color_blend(vuk::BlendPreset::eOff)
			   .set_rasterization(vuk::PipelineRasterizationStateCreateInfo{
				   .cullMode = vuk::CullModeFlagBits::eBack,
			   })
			   .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo{
				   .depthTestEnable = true,
				   .depthWriteEnable = true,
				   .depthCompareOp = vuk::CompareOp::eGreater,
			   })
			   .bind_index_buffer(*cmd.get_resource_buffer("indices"), vuk::IndexType::eUint32)
			   .bind_buffer(0, 0, "vertIndices")
			   .bind_buffer(0, 1, "vertices")
			   .bind_buffer(0, 2, "meshlets")
			   .bind_buffer(0, 3, "transforms")
			   .bind_buffer(0, 4, "instances");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, _viewProjection);
			
			cmd.draw_indexed_indirect(1, *cmd.get_resource_buffer("command"));
			
		}});
	
	visibility = vuk::Future(rg, "visibility/final");
	depth = vuk::Future(rg, "depth/final");
	
}

GET_SHADER(visibility_draw_vs);
GET_SHADER(visibility_draw_ps);
void Visibility::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto drawPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(drawPci, visibility_draw_vs, "visibility/draw.vs.hlsl");
	ADD_SHADER(drawPci, visibility_draw_ps, "visibility/draw.ps.hlsl");
	ctx.create_named_pipeline("visibility/draw", drawPci);
	
	m_compiled = true;
	
}
/*
void Worklist::compile(vuk::PerThreadContext& _ptc) {
	
	auto worklistPci = vuk::ComputePipelineBaseCreateInfo();
	worklistPci.add_spirv(std::vector<uint>{
#include "spv/visibility/worklist.comp.spv"
	}, "visibility/worklist.comp");
	_ptc.ctx.create_named_pipeline("visibility/worklist", worklistPci);
	
}

auto Worklist::create(Pool& _pool, Frame& _frame, vuk::Name _name,
	Texture2D _visbuf, TriangleList _triangles) -> Worklist {
	
	auto result = Worklist();
	
	result.tileDimensions = uint2{
		uint(ceil(float(_visbuf.size().x()) / float(TileSize.x()))),
		uint(ceil(float(_visbuf.size().y()) / float(TileSize.y()))) };
	
	// Create buffers
	
	constexpr auto InitialCount = uint4{0, 1, 1, 0};
	auto initialCounts = array<uint4, ListCount>();
	initialCounts.fill(InitialCount);
	
	result.counts = Buffer<uint4>::make(_frame.framePool, nameAppend(_name, "counts"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		initialCounts);
	
	result.lists = Buffer<uint>::make(_pool, nameAppend(_name, "tiles"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		usize(result.tileDimensions.x() * result.tileDimensions.y()) * ListCount);
	
	result.counts.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.lists.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	// Generate worklists
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "visibility/worklist"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_triangles.instances.resource(vuk::eComputeRead),
			result.counts.resource(vuk::eComputeRW),
			result.lists.resource(vuk::eComputeWrite) },
		.execute = [result, _visbuf, _triangles, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _visbuf, NearestClamp)
			   .bind_storage_buffer(0, 1, _triangles.indices)
			   .bind_storage_buffer(0, 2, _triangles.instances)
			   .bind_storage_buffer(0, 3, _frame.models.meshlets)
			   .bind_storage_buffer(0, 4, _frame.models.materials)
			   .bind_storage_buffer(0, 5, result.counts)
			   .bind_storage_buffer(0, 6, result.lists)
			   .bind_compute_pipeline("visibility/worklist");
			
			cmd.specialize_constants(0, uintFromuint16(_visbuf.size()));
			cmd.specialize_constants(1, ListCount);
			
			cmd.dispatch_invocations(_visbuf.size().x(), _visbuf.size().y());
			
		}});
	
	return result;
	
}
*/
}
